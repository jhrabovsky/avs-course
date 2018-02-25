_[22-02-2018]_

# CV2 - AVS

## TEORETICKÁ ČASŤ - ARP

- ARP:
    + _ČO_ to je?
    + _KEDY_ a _PREČO_ sa používa?
    + _AKO_ funguje?

- pre prácu so správami ARP je nevyhnutná znalosť a popis štruktúry ARP hlavičky: [FORMÁT ARP správy](./arp_format.png) a [VÝZNAM jednotlivých polí](./arp-hdr-fields.jpg) => detailný popis pre ARP je v _RFC 826_ (syntax, sémantika a stavový automat pre fungovanie ARP).

![Formát ARP správy](./arp_format.png)

![Význam polí ARP správy](./arp-hdr-fields.jpg)

- teoretický rozbor jednotlivých polí v hlavičke ARP => ich význam a možné použitie pre implementáciu nástroja `arping`, ktorý odosiela ARP žiadosti ohľadom konkrétnej IP adresy a prijíma (spracuje) príslušné odpovede.

- [!] konkrétny príklad deklarácie štruktúry pre ARP a rôznych podporných makier je uvedený v `net/if_arp.h` (prípadne v `linux/if_arp.h`) a `netinet/if_ether.h`.

## PRAKTICKÁ ČASŤ - PROGRAM

- [!] EtherType hodnota pre ARP je `0x0806`, je možné použiť aj makro `ETHERTYPE_ARP` z hlavičkového súboru `net/ethernet.h` 

- [!] pre jednoduchý prístup k dátam, ktoré sú umiestnené za hlavičkou, použijem `char Payload[0]` ako __posledný__ prvok štruktúry danej hlavičky => rieši problém prístupu k ďalšej hlavičke vo vnútri PDU (Protocol Data Unit):
    + (A) __smerník__ = zaberá 4 bajty. Tie sú navyše v danej štruktúre z pohľadu hlavičky, ktorú štruktúra reprezentuje (~ výplň) => nevhodný pre odosielanie po sieti.
    + (B) __pole s 0-veľkosťou__ = ukazuje na _adresu konca štruktúry_, teda na nasledujúcu hlavičku v správe. Tento prvok ale nezaberá žiadnu pamäť navyše (v porovnaní so smerníkom). Ide len o pomocný _zápis adresy na prvý bajt za štruktúrov_.

- Q: NAČO sú v ARP hlavičke žiadosti uvedené aj polia: `SRC_MAC` a `SRC_IP`?
    + A: Hodnoty týchto položiek si uloží prijímajúce zariadenie do svojej ARP tabuľky => odstránenie spätného ARP procesu (v opačnom smere), keďže pri generovaní odpovede potrebuje pôvodný cieľ MAC adresu pôvodného zdroja => [!] _obojsmerné_ zistenie MAC adries bez nutnosti vykonania ARP procesu pre spätný smer.

- [!] vždy skontrolujem minimálnu veľkosť rámca => minimum je _60B_ pre Ethernet => systém automaticky doplní FCS pole (Frame Check Sequence -> CRC).
- spôsoby vypísania chybovej správy na terminál:
    + ak funkcia zapíše chybový kód do premennej `errno`, použijem `perror()`.
    + inak použijem `fprintf(stderr,...)`, textová správa je odoslaná na chybový výstup `STDERR`.
- [!] pri alokácii dynamickej pamäte cez `malloc()` (napr. generovanie ARP žiadosti) nezabudnem na _uvoľnenie požičanej pamäte_ cez `free()` => správne upratovanie v programe môžem overiť cez nástroj `mtrace`.
- [!] vždy _inicializujem_ premennú => zabezpečenie _ZNÁMEJ_ mnou zvolenej počiatočnej hodnoty.

- načítanie MAC adresy z reťazca (vhodné v prípade čítania z terminálu - `STDIN`):
    + použijem funkciu `sscanf()`, kde uvediem špecifický žiadaný formát textu, v ktorom zapíšem MAC adresu (napr. v termináli).
    + požadované hodnoty z formátovaného textu vložím do jednotlivých položiek MAC adresy v pamäti.
    + tento prístup je vhodný pre všeobecný výber položiek z reťazca, teda napr. aj pre IP adresu.

- pre jednotlivé položky MAC adresy využijem _smerníkovú aritmetiku_ => `*(srcMAC + i)` = adresa i-teho bajtu zdrojovej MAC adresy (počítam od 0). Tento zápis je zhodný so zápisom: `srcMAC[i]`.

### PREVOD IP ADRESY: TEXT<->ČÍSLO

- IP adresa má dva formáty zápisu:
    + (A) 32-bitové binárne číslo (network-byte-order).
    + (B) text (bodkový zápis, ktorý používa 4 dekadické čísla z 0-255 oddelené bodkou).

- prevod medzi formátmi vykonám cez:
    + `inet_ntoa()` a `inet_aton()` => __ZASTARALÉ__, podporuje len IPv4 (použijem v programe na tomto cvičení).
    + `inet_ntop()` a `inet_pton()` => __NOVÉ__, podporujú IPv4 aj IPv6 adresy (použijem na ďalších cvičeniach).

- overím správnosť zadanej vstupnej IP adresy => overím oktetový zápis, pričom sa spolieham na návratovú hodnotu vyššie uvedených funkcií => použijem prevody medzi číselným a textovým formátom podľa účelu:
    + číselný formát je vhodný pre systém - dáta v pamäti.
    + textový formát je vhodný pre človeka - výpis na terminál.

-----

- _zachytenie odpovede_ na ARP žiadosti, ktoré sú určené pre mňa => sledujem prijaté správy a filtrujem (hľadám) _očakávané/požadované_ položky v ARP hlavičke (MAC a IP adresy).
    + pri spracovaní prijatých správ zvolím veľkosť dôležitých dát, ktoré musím spracovať (zvyšok ignorujem) => alokácia pamäte + čakanie na správu (nekonečný cyklus).

- pre jednoduchý prístup k hlavičke v prijatej správe vytvorím pomocnú premennú (jej typ zodpovedá štruktúre, ktorá reprezentuje ARP hlavičku v prijatom ramci) => postupne porovnávam jednotlivé položky =>
    1. _opcode_ (žiadosť/odpoveď),
    2. _srcIP_ pre žiadosť, _targetIP_ (dstIP) pre odpoveď.

- [!] `memcmp()` = porovnám hodnoty priamo v pamäti => vhodné pre IP adresy, ktoré majú vždy pevnú šírku (počet bitov).
- pre výpis pozitívnej odpovede použijem `printf()` s vhodným formátovaním IP a MAC adries => [!] použijem '*' pri vkladaní parametrov pre `printf()`, lebo požaduje hodnotu a nie adresu premennej.
    + [!] použijem `hh` formát, aby som prečítal len 1B namiesto 4B (platí pre `%x`) => pristupujem tak len k _1B_ pamäte => ochrana pred prístupom do neznámej časti pamäte mimo platný rozsah, napr. posledné 2 bajty MAC adresy.

- [!] pri generovaní ARP žiadosti a kontrolovaní ARP odpovede overím správnosť parametrov:
    + `response->OPCODE == ARP_RESP` (je ARP odpoveď),
    + `response->SRC_IP == request->TARGET_IP` (odpoveď prišla zo stanice, na ktorú som sa pýtal v mojej ARP žiadosti).

## VLÁKNA

- pridám `#include <pthread.h>`, aby som získal prístup k funkciam pre správu vlákien.
    + `man pthread_create` = popisuje vytvorenie a spustenie vlákna. Typ štartovacej funkcie sa musí zhodovať s typom, ktorý je uvedený v manuáli (funkčný prototyp pre `pthread_create()`) => `void * <názov-funkcie>(void * args);`.

- _oddelím_ generovanie ARP žiadosti od spracovania prijatých ARP odpovedí => použijem vlákna / procesy => _main()_ je hlavné vlákno, ktoré je zodpovedné za vytvorenie ďalších vlákien.
    + _main()_ = vykonáva odosielanie ARP žiadostí s oneskorením (`sleep(1)` pre 1 sekundu) v slučke => ochrana pred zahltením fyzického rozhrania.
    + _vlákno_ = prijíma ARP žiadosti, spracuje ich a vypíše aktuálny stav na obrazovku.

- Q: AKÉ sú rozdiely medzi vláknom a procesom?
    + A: Odlišujú sa v spôsobe a rozsahu zdieľania pamäte.

- použijem _globálne premenné_ pre zdieľanie premennej medzi viacerými vláknami (napr. id soketu).
    + _kritická sekcia_ = musím riešiť korektný prístup do zdieľanej premennej => semafor, mutex.
    + zvolím premenné, ktoré budú globálne => vyžadujú prístup z viacerých vlákien.
    + [!] __NEPOUŽÍVAŤ__ globálny smerník na lokálnu premennú, ktorá je definovaná vo vnútri fukcie => smerník neobsahuje vždy platnú hodnotu/adresu (napr. platí pre targetIP v `cv2_threads_arping.c`).

- [!] pri kompilácii pridám knižnicu pre podporu vlákien - `libpthread` => `-lpthread` (staršie kompilátory) alebo `-pthread` (novšie kompilátory).
- [ECLIPSE] pridanie knižnice `pthread`  => `Project -> Properties -> C/C++ General -> Paths & Symbols -> Libraries`.

## ROZŠIRUJÚCE ÚLOHY

- **ÚLOHA 1** = oskenujem cez `arping` celú lokálnu sieť => objavím a vypíšem na terminál všetky aktívne sieťové zariadenia v lokálnej sieti.
- **ÚLOHA 2** = pridám podporu pre _VLAN_ => vyžaduje pridanie značky (tagu) za `SRC_MAC` a pred `EtherType` v Ethernetovej hlavičke.
    + `VLAN-TAG` = `TAG-TYPE` (0x8100 pre 802.1Q - 16b), `PRI` (3b), `DEI` (= CFI - 1b), `VLAN-ID` (000 až FFF - 12b).
    + pridám `VLAN-TAG` do Ethernetovej hlavičky pred `EtherType`.
    + použijem len jednu premennú pre `PRI`, `DEI` a `VLAN-ID`, ktoré tak budú vyjadrené ako súčasť jedného 16-bitového čísla (2B) -> [PRI, DEI, VLAN-ID].

- **ÚLOHA 3** = __STP__ = generovanie STP rámcov s vhodným obsahom.
- **ÚLOHA 4** = __CDP__ = ohlasujem seba cez vhodne generované CDP rámce (alternatívou je __LLDP__).
- **ÚLOHA 5** = implementujem správanie _mostu_ (softvérový prepínač) => tejto téme sa budem venovať aj na ďalších cvičeniach.
    + nastavenie _promiskuitného režimu_ => príjem všetkých rámcov bez ohľadu na ich cieľovú MAC adresu.
    + zameranie na prepínaciu tabuľku => ukladanie, aktualizácia a vyhľadávanie => voľba vhodnej údajovej štruktúry (lineárne zreťazený zoznam s operáciami: pridaj, vymaž a vyhľadaj).
    + testovanie implementácie cez dva virtuálne stroje (Bridge a Klient1) a natívny systém (Klient2) cez premostenie prepojenia.
