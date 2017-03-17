_[17-03-2017]_

# CV4 - AVS

## MAN STRÁNKY

- `man 7 pthreads` = popis vlákien.
- `man pthread_rwlock_init`, `man pthread_rwlock_destroy`, `man pthread_rwlock_rdlock`, `man pthread_rdlock_wrlock` = manipulácia so zámkom, ktorý použijem pre riadenie prístupu vlákien do kritickej sekcie (prístup do BT).
- `man socketpair` = popis komunikačného spojenia (~ rúra).
    - prístup ku obom koncom linky cez dvojicu socketov (SP[0] = zápis, SP[1] = čítanie).
    - použitie len pre doménu __AF_UNIX__ => prenos údajov vo forme správ (typ socketu = __SOCK_DGRAM__). 
    - správu odošlem cez `send()` s flagom **MSG_DONTWAIT**, aby som neblokoval program.
    - správu prečítam cez `read()`.

## PROGRAM

### ROZŠÍRENIE PROGRAMU - BRIDGE

- zlepšenie vykonávania _mostu_ a rozšírenie jeho fcií:
    - rozdelím nezávislé časti programu do vlákien.
    - pridám pravidelné _čistenie_ tab (odstránim staré záznamy, ktorých platnosť už skončila). 

#### NOVÉ ROZLOŽENIE ÚLOH

- `FrameReaderThread` = príjem rámcov na zvolenom rozhraní.
    - každé rozhranie je obsluhované samostatným vláknom.
    - vlákno sa dozvie ID rozhrania zo vstupného argumentu, ktorý nastavím pri vytvorení vlákna.

- `AddRefreshMACThread` = proces učenia.
    - pridám do BT nové záznamy podľa srcMAC rámcov, ktoré príjmem na sledovaných rozhraniach.
    - komunikujem s vláknami (FrameReaderThread), ktoré mi posielajú srcMAC prijatých rámcov => komunikácia __N-to-1__.
    - použijem niektorý komunikačný prostriedok pre prenos údajov medzi vláknami => zvolil som `socketpair`.
 
- `Main` = inicializujem počiatočný stav programu:
    - nastavím rozhrania (vytvorím socket, nastavím adresu a zapnem promiskuitný režim),
    - vytvorím a pridelím každému rozhraniu jeden _socketpair_,
    - vytvorím a spustím vlákna pre _DeleteUnusedMACThread_ a _AddRefreshMACThread_,
    - pre každé rozhranie vytvorím a spustím vlákno _FrameReaderThread_.

- `DeleteUnusedMACThread` = pravidelne (~ 1 sec) odstránim staré záznamy z tab => _Q:_ AKO prikážem, aby fcia pozastavila vykonávanie príkazov a počkala tak požadovaný čas?

### POZNÁMKY K IMPLEMENTÁCII

- MAC adresa je aktuálne uložená ako 6-prvkové pole => vyžaduje kopírovanie po prvkoch (`for`) alebo kopírovanie bloku pamäťe (`memcpy`) => ak zmením reprezentáciu MAC adresy na celé číslo, nahradím kopírovanie za priradenie.
    - potrebujem údajový typ s veľkosťou aspoň 6B, ktorý je v programe vnímaný ako číslo => `unsigned long int`.   

- vizuálne zobrazenie aktuálneho stavu programu a zmien, ktoré nastanú (napr. pridanie záznamu do BT) pomáhajú pochopiť správanie programu a odhaliť chyby => použijem formátovaný výpis na obrazovku (`stdout`) s pekným zarovnaním => použijem pomocnú pamäť (pole znakov), kam si postupne vložím časti riadku. Obsah pamäte následne vypíšem.    
 
- ak nemením obsah vstupného argumentu vo fcii, použijem pred jeho typ `const` => kompilátor ma ochráni pred chybami, kedy neúmyselne mením hodnotu vo vnútri fcie => takto rozšírený typ je odlišný od pôvodného typu a preto musím vykonať pretypovanie (odstránim upozornenia od kompilátora). 

#### SPRACOVANIE VIACERÝCH POPISOVAČOV (FILE-DESCRIPTORS)

- `AddRefreshMACThread` komunikuje s viacerými vláknami súčasne cez `socketpair`. Problém prinášajú blokujúce operácie (`read`). Keď sa pokúsim prečítať údaje z jedného popisovača, vykonávanie vlákna sa zablokuje až do momentu, kým druhá strana pošle dáta. Spracovanie zvyšných popisovačov je tak zablokované aj v prípade, že mi niekto pošle údaje.  

- súčasné sledovanie viacerých popisovačov realizujem cez `select()` => blokujem vykonávanie, až kým aspoň na jednom zo sledovaných popisovačov neprídu údaje. Následne skontrolujem, ktoré z popisovačov sú aktívne (ponúkajú údaje na prečítanie) a tieto popisovače spracujem (prečítam čakajúce údaje). Tento postup opakujem v nekonečnej slučke. Pre sledovanie viacerých popisovačov cez `select` musím vo vnútri slučky vykonať:

    1. inicializujem (vynulujem) a nastavím zoznam sledovaných popisovačov => `fd_set`.
    2. zavolám fciu `select`.     
    3. prejdem všetkými sledovanými popisovačmi a kontrolujem ich aktivitu.
    4. spracujem aktívny popisovač.  

- Príklady: obsluha viacerých sieťových rozhraní, obsluha viacerých komunikačných prostriedkov (môj prípad komunikácie N-to-1 cez `socketpair`).  

- `select` môžem použiť ako relatívne presný časovač vo vláknach => nevložím žiadny zoznam popisovačov, ale nastavím čas pre timeout (posledný argument).

-----
- pravidelné vykonávanie fcie cez časovač (**TIMER**) so zvolenou periódou, napr. čistenie BT.

-----

#### SYNCHRONIZÁCIA VLÁKIEN

- zvolím všetky zdroje vo forme premenných, ku ktorým pristupujú viaceré vlákna => použijem globálne premenné, aby k nim mali prístup všetky vlákna => __adresa BT__, __celkový počet rozhraní__, __detailné údaje o rozhraniach__, ... 
- _synchronizácia prístupu_ rôznych vlákien do kritickej sekcie (časť programu, kde pristupujem k zdieľanému zdroju, napr. spoločná pamäť):
    -  **mutex** = neodlišuje operácie čítania a zápisu => obe operácie majú rovnakú prioritu.
    -  **phtread_rwlock_t** = zámok, ktorý odlišuje operácie zápisu a čítania => zápis má vždy vyššiu prioritu ako čítanie. Dôvodom je rôzna úroveň bezpečnosti - čítanie je nedeštruktívna operácia, ktorá nemodifikuje žiadne údaje, zápis je naopak deštruktívna operácia, ktorej zlyhanie znehodnotí modifikované údaje. Viaceré vlákna môžu súčasne čitať údaje v rôznom poradí, ale zapisovať môže vždy len jedno vlákno.

__Zámok (pthread_rwlock_t)__

- pred použitím zámku ho musím vždy najprv inicializovať:
    - statická inicializácia = pri deklarácii zámku mu priradím MAKRO `PTHREAD_RWLOCK_INITIALIZER` (~ mutex), ktoré použije defaultne nastavenia.
    - dynamická inicializácia = použijem fciu `pthread_rwlock_init()`, ktorá nastaví zámok podľa vstupných argumentov.

- zámok odlišuje operácie čitania a zápisu => pred vstupom do krytickej sekcie musím zámok zamknúť podľa operácie ktorú budem vykonávať nad zdieľaným zdrojom => odlišný spôsob zamykania:
    - čítanie = `pthread_rwlock_rdlock()`).
    - zápis = `pthread_rwlock_wrlock()`.
- po ukončení kritickej sekcie zámok opäť odomknem cez `pthread_rwlock_unlock()`.

### ČISTENIE BT

1. zamknem zámok pre zápis do tab => `pthread_rwlock_wrlock()`,
2. zaznamenám si aktuálny čas => `time(NULL)`,
3. odstránim všetky staré záznamy => prejdem celú BT a pre každý záznam overím jeho aktuálny vek,
4. odomknem zámok nad tab => `pthread_rwlock_unlock()`,
5. počkám pevný časový interval (~1 sec) => regulácia pravidelného čistenia, aby som touto operáciou nezahltil CPU,
6. vrátim sa na krok 1.

## TESTOVACIE PROSTREDIE

- TOPÓLÓGIA: PC1 -- (net1) --> Bridge <-- (net2) -- PC2, kde:
    - net1 a net2 sú _internal network_,
    - PC1 a PC2 sú _VM (MiniDebian)_ => pridelené IP adresy zo spoločnej siete 192.168.1.0/24,
    - Bridge je VM, kde je spustený program "bridge" => eth0 a eth1 nemajú pridelené IP adresy a sú aktivované cez `ip link set <rozhranie> up`.

## DOPLŇUJÚCE ÚLOHY 

- **ÚLOHA 1** = pridaj _riadiacu konzolu_ do programu => vytvor _MENU_ so základnými fciami, cez ktoré môže používateľ meniť nastavenia mostu.
- **ÚLOHA 2** = pridaj podporu pre VLAN => prístupové virtuálne siete (VLAN) a značkovanie rámcov (_VLAN-TAGGING_).
