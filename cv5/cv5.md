_[18-03-2017]_

# CV5 - AVS

## TEÓRIA - KNIŽNICE PRE PRÍJEM A ODOSIELANIE VLASTNÝCH RÁMCOV

- témou cvičenia je praktická ukážka knižníc `libnet` a `libpcap`, ktoré nie sú závislé od OS.
    + __LIBNET__ = ponúka fcie pre generovanie rámcov s vlastným obsahom a ich odosielanie. Knižnica využíva interne surové sockety.
    + __LIBPCAP__ = ponúka fcie pre príjem (odchytávanie) rámcov, pričom podporuje použitie __BSD filtrov__, ktoré používajú mnohé nástroje (tcpdump, wireshark a iné).

## PROGRAM

- vytvorím program z CV2 (ARP), v ktorom nahradím prácu so surovými socketmi za použitie fcií s knižníc _libnet_ a _libpcap_.
- základné úlohy: 
    + pravidelné odosielanie ARP žiadostí na IP adresu, ktorú zadám ako vstupný argument pri spustení programu.
    + príjem rámcov, overenie ARP odpovedí na mnou posielané ARP žiadosti.
    + oznámenie v prípade zhody => výpis stručnej správy na obrazovku.

### LIBPCAP

- nainštalujem balík `libpcap-dev`, aby som získal prístup ku knižnici => získam hlavičkový súbor _pcap/pcap.h_. 
- `man pcap` => popis fcií a poradie, v akom ich mám používať.

- Postup pre odchytávanie rámcov na sledovanom rozhraní:
    1. zvolím sieťové rozhranie, na ktorom idem odchytávať rámce => použijem názov rozhrania.
        - ak názov rozhrania nepoznám, vyhľadám použiteľné rozhranie => `pcap_lookupdev()`, `pcap_findalldevs()`.
    2. deklarujem smerník na objekt (`pcap_t *`) pre manipuláciu s rozhraním => vytvorím objekt `pcap_create()`.
        - ak chcem odchytávať prevádzku na všetkých rozhraniach, použijem `NULL` ako argument pre názov rozhrania. 
    3. nastavím rozširujúce vlastnosti odchytávania, napr. _promiskuitný režim_ (`pcap_set_promisc()`).
    4. aktivujem odchytávanie => `pcap_activate()`.
    5. prečítam a spracujem prichádzajúce rámce => `pcap_next()`, `pcap_next_ex()`,  `pcap_dispatch()`, `pcap_loop()`.
    6. zatvorím popisovač cez `pcap_close()`.

- `struct pcap_pkthdr` obsahuje metadata o prijatom rámci (čas príchodu, dĺžka uloženej časti rámca, celková dĺžka).
- `pcap_dispatch()` a `pcap_loop()` používajú "callback" fciu, ktorá sa automaticky použije pre spracovanie každého z prijatých rámcov => `void (*pcap_handler)(u_char * user, const struct pcap_pkthdr * pktHdr, const u_char *pkt)`.
    + počet odchytených rámcov nastavím v 2. argumente (0 = nekonečno).

- __[!]__ pridám knižnicu _pcap_ do ECLIPSE alebo použijem `-lpcap`  pri kompilácii zdrojového kódu z príkazového riadku.

### LIBNET

- nainštalujem balík `libnet1-dev`, aby som získal prístup ku knižnici a k jej fciam cez hlavičkový súbor _libnet.h_.
- knižnica nemá manuálové stránky priamo pre každú z fcií => zoznam fcií získam cez `man libnet-functions.h`.

- Postup pre vytvorenie a odoslanie vlastného rámca:
    1. deklarujem smerník na štruktúru `struct libnet_t`, ktorá reprezentuje kontext pre LIBNET (mnou vytvorený rámec).
    2. inicializujem kontext => `libnet_init()` => uvediem typ rámca (`LIBNET_LINK`, `LIBNET_RAW4` a iné), ktorý idem vyskladať a názov rozhrania, cez ktoré plánujem rámec odoslať (NULL = rozhranie je zvolené systémom).
    3. deklarujem TAGY, do ktorých uložím hlavičky rámca => `libnet_ptag_t`.
    4. nastavím obsah hlavičiek => každý protokol má vlastný formát hlavičky, preto existuje pre každý protokol samostatná fcia pre vytvorenie a nastavenie jeho hlavičky => `libnet_build_<protokol>()`, `libnet_autobuild_<protokol>()`. Druhý typ fcií nastaví viaceré polia hlavičky na predvolené hodnoty. __[!]__ Hlavičky nastavujem vždy v poradí: L7 -> L2.
        - Príklad: ARP hlavička (`libnet_build_arp()`) -> Ethernet hlavička (`libnet_build_ethernet()`).
    5. odošlem rámec => `libnet_write()`.
    6. zruším kontext => `libnet_destroy()`.   

- `libnet_geterror()` vráti textový popis poslednej zaznamenanej chyby, ktorá vznikla volaním fcie pre libnet kontext.
- Ethernetovú hlavičku nastavujem len v prípade kontextu typu __LIBNET_LINK__ a __LIBNET_LINK_ADV__ => rámec má rovnaké správanie ako v prípade socketu z domény __AF_PACKET__ typu __SOCK_RAW__.
- __[!]__ pridám knižnicu _net_ do ECLIPSE alebo použijem `-lnet`  pri kompilácii zdrojového kódu z príkazového riadku.

#### PRÍKLAD - ARP ŽIADOSŤ
 
1. vytvorím kontext pre libnet typu LIBNET_LINK, keďže ARP nepoužíva IPv4 => `libnet_init()`.
2. nastavím ARP hlavičku => `libnet_autobuild_arp()`.
3. nastavím ETHERNET hlavičku => `libnet_autobuild_ethernet()`.
4. odošlem rámec => `libnet_write()`.
5. zatvorím kontext => `libnet_destroy()`.  

- pre jednoduchosť nastavím hlavičky cez 2. typ fcií => knižnica nastaví viaceré hodnoty za mňa.
- _IP a MAC adresy_ zadávam v __NETWORK_BYTE_ORDER__.
- _OPCODE_ a _TYPE_ zadávam v __HOST_BYTE_ORDER__.
