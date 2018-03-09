_[09-03-2017]_

# CV4 - AVS

## MAN STRÁNKY

- `man 7 pthreads` = popis vlákien.
- `man pthread_rwlock_init`, `man pthread_rwlock_destroy`, `man pthread_rwlock_rdlock`, `man pthread_rwlock_wrlock` = manipulácia so zámkom, ktorý použijem pre riadenie prístupu vlákien do kritickej sekcie (prístup do BT).
- `man socketpair` = popis komunikačného spojenia (~ rúra).
    + prístup ku obom koncom linky cez dvojicu socketov (`SP[0]` = zápis, `SP[1]` = čítanie).
    + použitie len pre doménu `AF_UNIX` => prenos údajov vo forme správ (typ socketu `SOCK_DGRAM`).
    + správu odošlem cez `send()` s flagom `MSG_DONTWAIT`, aby som neblokoval program.
    + správu prečítam cez `read()`.

## PROGRAM

### ROZŠÍRENIE PROGRAMU - BRIDGE

- zlepšenie vykonávania _mostu_ a rozšírenie jeho funkcií:
    + rozdelím nezávislé časti programu do vlákien.
    + pridám pravidelné _čistenie_ BT => odstránim staré záznamy, ktorých platnosť už skončila.

#### NOVÉ ROZLOŽENIE ÚLOH

- `FrameReaderThread` = príjem rámcov na zvolenom rozhraní.
    + každé rozhranie je obsluhované samostatným vláknom.
    + vlákno sa dozvie ID rozhrania zo vstupného argumentu, ktorý nastavím pri vytvorení vlákna.

- `AddRefreshMACThread` = proces učenia.
    + pridám do BT nové záznamy podľa `srcMAC` rámcov, ktoré príjmem na sledovaných rozhraniach.
    + komunikujem s vláknami (`FrameReaderThread`), ktoré mi posielajú `srcMAC` prijatých rámcov => komunikácia **N-to-1**.
    + použijem niektorý komunikačný prostriedok pre prenos údajov medzi vláknami => zvolil som `socketpair`.

- `Main` = inicializujem počiatočný stav programu:
    + nastavím rozhrania (vytvorím socket, nastavím adresu a zapnem promiskuitný režim),
    + vytvorím a pridelím každému rozhraniu jeden `socketpair`,
    + vytvorím a spustím vlákna pre `DeleteUnusedMACThread` a `AddRefreshMACThread`,
    + pre každé rozhranie vytvorím a spustím vlákno `FrameReaderThread`.

- `DeleteUnusedMACThread` = pravidelne (~ 1 sec) prechádzam BT a odstránim staré záznamy => _Q:_ AKO prikážem, aby funkcia pozastavila vykonávanie príkazov a počkala požadovaný časový interval?

### POZNÁMKY K IMPLEMENTÁCII

- MAC adresa je aktuálne uložená ako 6-prvkové pole => vyžaduje kopírovanie po prvkoch (`for`) alebo kopírovanie bloku pamäťe (`memcpy`) => ak zmením reprezentáciu MAC adresy na celé číslo, nahradím kopírovanie za priradenie.
    + potrebujem údajový typ s veľkosťou aspoň 6B, ktorý je v programe vnímaný ako číslo => `unsigned long int`.

- vizuálne zobrazenie aktuálneho stavu programu a zmien, ktoré nastanú (napr. pridanie záznamu do BT) pomáhajú pochopiť správanie programu a odhaliť chyby => použijem formátovaný výpis na obrazovku (`stdout`) s pekným zarovnaním => použijem pomocnú pamäť (pole znakov), kam si postupne vložím časti riadku. Obsah pamäte následne vypíšem.

- ak nemením obsah vstupného argumentu vo funkcii, použijem pred jeho typ `const` => kompilátor ma ochráni pred chybami, kedy neúmyselne mením hodnotu vo vnútri funkcie => takto rozšírený typ je odlišný od pôvodného typu a preto musím vykonať pretypovanie (odstránim upozornenia od kompilátora).

#### SPRACOVANIE VIACERÝCH POPISOVAČOV (FILE-DESCRIPTORS)

- `AddRefreshMACThread` komunikuje s viacerými vláknami súčasne cez `socketpair`. Problém prinášajú blokujúce operácie (`read`). Keď sa pokúsim prečítať údaje z jedného popisovača, vykonávanie vlákna sa zablokuje až do momentu, kým druhá strana pošle dáta. Spracovanie zvyšných popisovačov je tak zablokované aj v prípade, že mi niekto pošle údaje.

- súčasné sledovanie viacerých popisovačov realizujem cez `select()` => blokujem vykonávanie, až kým aspoň na jednom zo sledovaných popisovačov neprídu údaje. Následne skontrolujem, ktoré z popisovačov sú aktívne (ponúkajú údaje na prečítanie) a tieto popisovače spracujem (prečítam čakajúce údaje). Tento postup opakujem v nekonečnej slučke. Pre sledovanie viacerých popisovačov cez `select` musím vo vnútri slučky vykonať:

    1. inicializujem (vynulujem) a nastavím zoznam sledovaných popisovačov => `fd_set`.
    2. zavolám fciu `select`.
    3. prejdem všetkými sledovanými popisovačmi a kontrolujem ich aktivitu.
    4. spracujem aktívny popisovač.

- Príklady: obsluha viacerých sieťových rozhraní, obsluha viacerých komunikačných prostriedkov (môj prípad komunikácie N-to-1 cez `socketpair`).
- `select` môžem použiť ako relatívne presný časovač vo vláknach => nevložím žiadny zoznam popisovačov, ale nastavím čas pre timeout (posledný argument).
    + alternatívou je použitie časovača (**TIMER**) so zvolenou periódou, ktorý pravidelne vykonáva konkrétnu funkciu, napr. čistenie BT.

#### SYNCHRONIZÁCIA VLÁKIEN

- zvolím všetky zdroje (premenné), ku ktorým pristupujú viaceré vlákna => použijem globálne premenné, aby k nim mali prístup všetky vlákna => **adresa BT**, **celkový počet rozhraní**, **detailné údaje o rozhraniach**, ...
- _synchronizácia prístupu_ rôznych vlákien do **kritickej sekcie** (časť programu, kde pristupujem k zdieľanému zdroju, napr. spoločná pamäť):
    +  **mutex** = neodlišuje operácie čítania a zápisu => obe operácie majú rovnakú prioritu.
    +  **phtread_rwlock_t** = zámok, ktorý odlišuje operácie zápisu a čítania => zápis má vždy vyššiu prioritu ako čítanie. Dôvodom je rôzna úroveň bezpečnosti - čítanie je nedeštruktívna operácia, ktorá nemodifikuje žiadne údaje, zápis je naopak deštruktívna operácia, ktorej zlyhanie znehodnotí modifikované údaje. Viaceré vlákna môžu súčasne čitať údaje v rôznom poradí, ale zapisovať môže vždy len jedno vlákno.

##### ZÁMOK (pthread_rwlock_t)

- pred použitím zámku ho musím vždy najprv inicializovať:
    + **statická** inicializácia = pri deklarácii zámku mu priradím MAKRO `PTHREAD_RWLOCK_INITIALIZER` (~ mutex), ktoré použije defaultné nastavenia.
    + **dynamická** inicializácia = použijem funkciu `pthread_rwlock_init()`, ktorá nastaví zámok podľa vstupných argumentov.

- zámok odlišuje operácie čitania a zápisu => pred vstupom do kritickej sekcie musím zámok zamknúť podľa operácie, ktorú budem vykonávať nad zdieľaným zdrojom => odlišný spôsob zamykania:
    + čítanie = `pthread_rwlock_rdlock()`.
    + zápis = `pthread_rwlock_wrlock()`.
- po ukončení kritickej sekcie zámok opäť odomknem cez `pthread_rwlock_unlock()`.

### ČISTENIE BT

1. zamknem zámok pre zápis do BT => `pthread_rwlock_wrlock()`,
2. zaznamenám si aktuálny čas => `time(NULL)`,
3. odstránim všetky staré záznamy => prejdem celú BT a pre každý záznam overím jeho aktuálny vek,
4. odomknem zámok nad tab => `pthread_rwlock_unlock()`,
5. počkám pevný časový interval (~1 sec) => regulácia pravidelného čistenia, aby som touto operáciou nezahltil CPU,
6. vrátim sa na krok 1.

## DOPLŇUJÚCE ÚLOHY

- **ÚLOHA 1** = pridaj _riadiacu konzolu_ do programu => vytvor _MENU_ so základnými funkciami, cez ktoré môže používateľ meniť nastavenia mostu.
- **ÚLOHA 2** = pridaj podporu pre VLAN => prístupové virtuálne siete (VLAN) a značkovanie rámcov (_VLAN-TAGGING_).
