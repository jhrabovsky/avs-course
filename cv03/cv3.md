_[03-03-2018]_

# CV3 - AVS

## TEÓRIA - ZADANIE PROBLÉMU

- témou cvičenia je softvérový prepínač (_BRIDGE_) a implementácia jeho funkcií do vlastného programu.
- záznamy o zdrojových MAC adresách prijatých rámcov ukladám do _prepínacej tabuľky_ (Bridge Table = **BT**).
    + každý záznam (riadok) v BT obsahuje MAC adresu a rozhranie, za ktorým sa nachádza zariadenie s touto MAC adresou.
- všetky prijaté rámce sú spracované a odoslané ďalej výstupným rozhraním podľa BT.
- základné operácie mostu:
    + **UČENIE** = príjmem rámec a aktualizujem BT:
        1. Ak sa **zdrojová** MAC adresa rámca v BT ešte nenachádza, vytvorím nový záznam a vložím ho do BT.
        2. Inak vyberiem existujúci záznam a prepíšem rozhranie v zázname na vstupné rozhranie prijatého rámca.
    + **PREPÍNANIE** = vyhľadám záznam v BT podľa **cieľovej** MAC adresy rámca:
        1. Ak záznam existuje, odošlem rámec len na rozhranie uvedené v zázname.
        2. Inak odošlem rámec _všetkými_ rozhraniami. **[!] Rámec NIKDY neodosielam na rozhranie, na ktorom som tento rámec prijal.**

## PROGRAM

- vytvorím údajovú štruktúru **OBOJSMERNÝ LINEÁRNE ZREŤAZENÝ ZOZNAM** => navrhnem a realizujem základné fcie pre manipuláciu s údajovou štruktúrou:
    + vytvorenie prázdnej tabuľky,
    + pridanie záznamu do tabuľky,
    + vyhľadanie záznamu v tabuľke podľa hodnoty (kľúč),
    + odstránenie záznamu z tabuľky,
    + odstránenie tabuľky.

- operácie mostu môžem realizovať:
    + (A) _sekvenčne_ v hlavnej funkcii (`main`),
    + (B) _paralelne_ v samostatných vláknach (úloha pre CV4).

### PREPÍNACIA TABUĽKA

- lineárne zreťazený zoznam vyžaduje definovanie základných štruktúr, ktoré použijem pre implementáciu jeho operácii.

- základné štruktúry:
    + `IntDescriptor` = obsahuje údaje o sieťovom rozhraní:
        * názov rozhrania,
        * index rozhrania,
        * popisovač prislúchajúceho soketu.
    + `MACAddress`,
    + `BTEntry` = položka prepínacej tabuľky, ktorá obsahuje:
        * odkaz na predchádzajúcu položku,
        * odkaz na nasledujúcu položku,
        * MAC adresu,
        * čas poslednej aktualizácie,
        * odkaz na sieťové rozhranie.
    + `EthFrame`.

- základné fcie:
    + `CreateBTEntry`,
    + `InsertBTEntry`,
    + `AppendBTEntry`,
    + `FindBTEntry`,
    + `EjectBTEntry`,
    + `DestroyBTEntry`,
    + `PrintBT`,
    + `FlushBT`,

- `UpdateOrAddMACEntry` = vykoná aktualizáciu BT podľa procesu učenia.
- odkaz na začiatok BT sa nachádza vo virtuálnom prvku `HEAD`, ktorý je prvým prvkom BT, ale neobsahuje platné údaje.
- `time()` = získam aktuálny čas systému (v sekundách od začiatku epochy - 1.1.1970) => `man 2 time`.

### NASTAVENIE ROZHRANÍ

- súčasťou inicializácie programu je nastavenie každého sledovaného rozhrania:
    + vyplním detailné údaje o rozhraní, ktoré uložím do štruktúry popisujúcej sieťové rozhranie (`struct IntDescriptor`).
    + vytvorím soket z domény `AF_PACKET`.
    + prepojím soket s fyzickým rozhraním podľa jeho názvu, ktorý budem zadávať ako vstupný argument pri spustení programu => použijem adresu `struct sockaddr_ll`.
    + nastavím promiskuitný režim rozhrania => cez IFR (`man 7 netdevice`) => `struct ifr`:
        1. načítam aktuálne nastavenia príznakov pre rozhranie,
        2. pridám do príznakov aj `IFF_PROMISC` cez bitovú operáciu OR (`|`),
        3. nastavím nové príznaky pre rozhranie.

### SPRACOVANIE RÁMCOV

- obsluhujem prichádzajúce rámce v nekonečnej slučke tak, ako vyžaduje správanie mostu.
- obsluha viacerých soketov súčasne vyžaduje neblokujúcu operáciu `read()` (príjem rámca) => použijem `select()` nad soketmi, ktorým môžem sledovať viacero rozhraní súčasne (ďalšie informácie v CV4).
    + `FD_ZERO`, `FD_SET`, `FD_ISSET` = makrá dostupné pre manipuláciu s `select()` => (`man select`).
- _read()_ = štandardne blokujúca operácia => program čaká (je blokovaný), kým neprídu údaje (rámce) zo siete.
- _write()_ = štandardne neblokujúca operácia => odoslanie rámca je za normálnych okolností okamžité.

## TESTOVACIE PROSTREDIE

- TOPOLÓGIA: PC1 -- (net1) --> Bridge <-- (net2) -- PC2, kde:
    + net1 a net2 sú _internal network_,
    + PC1 a PC2 sú _VM (MiniDebian)_ => pridelené IP adresy zo spoločnej siete 192.168.1.0/24, [!] potrebné nastaviť promiskuitný režim siete pre všetky virtuálne stroje na `Allow All`.
    + Bridge je VM, kde je spustený program pre prepínanie => eth0 a eth1 nemajú pridelené IP adresy a sú aktivované cez `ip link set <rozhranie> up`.

## ROZŠIRUJÚCE ÚLOHY

- pravidelné _čistenie BT_ => odstránenie záznamov, ktore sú už staré.
- realizácia operácii mostu vo viacerých vláknach:
    + príjem rámca a jeho odoslanie podľa cieľovej MAC adresy v BT na výstupné rozhranie.
    + aktualizácia BT podľa zdrojovej MAC adresy prijatého ramca.
    + vlákna vzájomne komunikujú => zvolím jeden alebo viacero komunikačných prostriedkov pre výmenu údajov.
