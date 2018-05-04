_[04-05-2018]_

# Libnet a Libpcap - Knižnice pre príjem a odosielanie vlastných rámcov

## Teória

- témou cvičenia je praktická ukážka knižníc `libnet` a `libpcap`, ktoré nie sú závislé od OS.
    + __LIBNET__ = ponúka funkcie pre generovanie rámcov s vlastným obsahom a ich odosielanie, pričom interne sú použité surové sokety (`SOCK_RAW`).
    + __LIBPCAP__ = ponúka funkcie pre príjem (odchytávanie) rámcov. Knižnica podporuje použitie __BSD filtrov__, ktoré sú využívané v mnohých nástrojoch (tcpdump, wireshark a iné).

## Program

- vytvoríme program z CV2 (ARP), v ktorom nahradíme prácu so surovými soketmi za použitie funkcií z knižníc _libnet_ a _libpcap_.
- základné úlohy:
    + pravidelné odosielanie ARP žiadostí na IP adresu, ktorú zadám ako vstupný argument pri spustení programu.
    + príjem rámcov, overenie ARP odpovedí na mnou posielané ARP žiadosti.
    + oznámenie v prípade zhody => výpis stručnej správy na obrazovku.

### LIBPCAP

- nainštalujeme balík `libpcap-dev`, aby sme získali prístup ku knižnici => získame hlavičkový súbor _pcap/pcap.h_.
- `man pcap` = popis funkcií a poradie, v akom ich máme používať.

- Postup pre odchytávanie rámcov na sledovanom rozhraní:
    1. zvolíme sieťové rozhranie, na ktorom ideme odchytávať rámce => použijeme názov rozhrania (získaný napríklad z výpisu `ip link` v termináli).
        + ak názov rozhrania nepoznáme, vyhľadáme použiteľné rozhranie => `pcap_lookupdev()`, `pcap_findalldevs()`.
    2. deklarujeme smerník na objekt (`pcap_t *`) pre manipuláciu s rozhraním => vytvoríme objekt `pcap_create()`.
        + ak chceme odchytávať prevádzku na všetkých rozhraniach, použijeme `NULL` ako argument pre názov rozhrania.
    3. nastavíme rozširujúce vlastnosti odchytávania, napr. _promiskuitný režim_ (`pcap_set_promisc()`).
    4. aktivujeme odchytávanie => `pcap_activate()`.
    5. prečítame a spracujem prichádzajúce rámce => `pcap_next()`, `pcap_next_ex()`,  `pcap_dispatch()`, `pcap_loop()`.
    6. zatvoríme popisovač cez `pcap_close()`.

- `struct pcap_pkthdr` obsahuje metadata o prijatom rámci (čas príchodu, dĺžka uloženej časti rámca, celková dĺžka).
- `pcap_dispatch()` a `pcap_loop()` používajú "callback" funkciu, ktorá sa automaticky použije pre spracovanie každého z prijatých rámcov => `void (*pcap_handler)(u_char * user, const struct pcap_pkthdr * pktHdr, const u_char *pkt)`.
    + počet odchytených rámcov nastavíme v druhom argumente (`0` = nekonečno).

- __[!]__ pridáme knižnicu _pcap_ do ECLIPSE alebo použijeme `-lpcap`  pri kompilácii zdrojového kódu v termináli.

### LIBNET

- nainštalujeme balík `libnet1-dev`, aby sme získali prístup ku knižnici a k jej funkciam cez hlavičkový súbor _libnet.h_.
- knižnica nemá manuálové stránky priamo pre každú z funkcií => zoznam funkcií získame cez `man libnet-functions.h`. Alternatívou je inštalácia balíka `libnet1-doc`, ktorý ponúka prehľadnú webovú stránku umiestnenú lokálne v `/usr/share/doc/libnet1-doc/html/`.

- Postup pre vytvorenie a odoslanie vlastného rámca:
    1. deklarujeme smerník na typ `libnet_t`, ktorý reprezentuje kontext pre LIBNET (mnou vytvorený rámec).
    2. inicializujeme kontext => `libnet_init()` => uvedieme typ rámca (`LIBNET_LINK`, `LIBNET_RAW4` a iné), ktorý ideme vyskladať a názov rozhrania, cez ktoré plánujeme odoslať rámec (`NULL` = rozhranie je zvolené systémom).
    3. deklarujeme TAGY, do ktorých uložíme jednotlivé hlavičky rámca => `libnet_ptag_t`.
    4. nastavíme obsah hlavičiek => každý protokol má vlastný formát hlavičky, preto existuje pre každý protokol samostatná funkcia pre vytvorenie a nastavenie jeho hlavičky => `libnet_build_<protokol>()`, `libnet_autobuild_<protokol>()`. Druhý typ funkcií nastaví viaceré polia hlavičky automaticky na predvolené hodnoty.
        + __[!]__ Hlavičky nastavujeme vždy v poradí: L7 -> L2.
        + Príklad: ARP hlavička (`libnet_build_arp()`) -> Ethernet hlavička (`libnet_build_ethernet()`).
    5. odošleme rámec => `libnet_write()`.
    6. zrušíme kontext => `libnet_destroy()`.

- `libnet_geterror()` vráti textový popis poslednej zaznamenanej chyby, ktorá vznikla volaním funkcie pre libnet kontext.
- Ethernetovú hlavičku nastavujeme len v prípade kontextu typu `LIBNET_LINK` a `LIBNET_LINK_ADV` => rámec má rovnaké správanie ako v prípade soketu z domény `AF_PACKET` typu `SOCK_RAW`.
- __[!]__ pridáme knižnicu _net_ do ECLIPSE alebo použijeme `-lnet`  pri kompilácii zdrojového kódu v termináli.

#### Príklad - ARP žiadosť

1. vytvoríme kontext pre libnet typu LIBNET_LINK, keďže ARP nepoužíva IPv4 => `libnet_init()`.
2. nastavíme ARP hlavičku => `libnet_autobuild_arp()`.
3. nastavíme ETHERNET hlavičku => `libnet_autobuild_ethernet()`.
4. odošleme rámec => `libnet_write()`.
5. zatvoríme kontext => `libnet_destroy()`.

- _IP a MAC adresy_ zadávame v **NETWORK_BYTE_ORDER**.
- _OPCODE_ a _TYPE_ zadávame v **HOST_BYTE_ORDER**.
