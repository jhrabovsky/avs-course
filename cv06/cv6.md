---
title: "AVS - CV6: UDP"
author: [Jakub Hrabovský, Martin Kontšek]
date: "2017-03-24"
...

# AVS - CV6: UDP

## Zdroje

- `man 7 ip` - všeobecne popisuje implementáciu IPv4 v linuxe => použitie socketov z domény __AF_INET__.
    + obsahuje kroky pre povolenie posielania a prijímania broadcastových a multicastových správ.
- `man 7 udp` - popisuje sockety z domény `AF_INET` typu `SOCK_DGRAM`.
- `man 7 socket` - popisuje spôsob načítania aktuálnych volieb socketov a nastavenia nových volieb => _getsockopt()_ a _setsockopt()_.

## Teoretická časť - UDP Sokety

- UDP protokol je v linuxe dostupný cez sockety z domény `AF_INET` typu `SOCK_DGRAM` => 3. argument pri vytvorení socketu je štandardne _0_.
    + _nespojovo-orientovany_ - komunikácia prebieha formou správ _bez vytvorenia relácie_ medzi koncovými zariadeniami => pri odosielaní a prijímani každej správy musím uviesť adresu (`struct sockaddr_in`) cieľovej stanice (alternatívou je použitie fcie `connect()`).
    + nepodporuje _potvrdzovanie_ správ.
    + neponúka _riadenie_ toku a zahltenia.
    + _zachováva hranice_ medzi správami. => dáta odoslané cez fciu _sendto()_ a jej alternatívy sú odoslané do siete v samostatných segmentoch.

- __FRAGMENTÁCIA__ => UDP nepozná __MTU__ na ceste medzi komunikujúcimi zariadeniami => ak nechcem používať fragmentáciu na IP, som zodpovedný za nastavenie a dodržiavanie maximálnej veľkosti UDP segementov tak, aby príslušný rámec nepresiahol veľkosť použitej prenosovej technológie (pre Ethernet - 1518B).

## Praktická časť - Program

- praktická časť cvičenia pozostáva z realizácie 2 programov:
    + UDP chat klient.
    + UDP chat server.

### Server

1. vytvorím socket z domény AF_INET a typu SOCK_DGRAM => `socket()`.
2. nastavím hodnoty mojej adresy (IP adresa a lokálny UDP port) a prepojím adresu so socketom => `bind()`.
3. prijímam správy od klientov a reagujem odoslaním odpovedí => `recvfrom()`, `sendto()`.
4. uzatvorím socket => `close()`.

### Klient

1. vytvorím socket z domény AF_INET a typu SOCK_DGRAM => `socket()`.
2. (voliteľné) nastavím hodnoty mojej adresy (IP adresa a lokálny UDP port) a prepojím adresu so socketom => `bind()`.
3. (voliteľné) vytvorím virtuálne spojenie s druhou stranou => umožní používať aj fcie `send()`, `write()`, `recv()`, `read()`.
4. odosielam správy na server a prijímam odpovede => `sendto()`, `recvfrom()`.
5. uzatvorím socket => `close()`.

### Broadcast

- ak potrebujem posielať správy na _broadcast_ MAC adresu => povolím cez `setsockopt()` voľbu `SO_BROADCAST` => `sesockopt(fd, SOL_PACKET, SO_BROADCAST, ...)`.

### Multicast

- pre posielanie a príjem multicastových správ povolím cez `setsockopt()` => pripojím sa do multicastovej skupiny cez `IP_ADD_MEMBERSHIP` => použijem `struct ip_mreqn` => `setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, ...)`.
    + ako lokálnu IP adresu v štruktúre môžem použiť `INADDR_ANY` => systém zvolí sám.

### Poznámky

- __[!]__ len 1 proces môže používať dvojicu [IP]:[PORT] => aktuálne bežiace služby musím odlíšiť cez rôzne porty.
- __[!]__ proces môže počúvať a komunikovať, až keď má pridelený svoj lokálny port => _(A)_ nastavím staticky v adrese `struct sockaddr_in`, _(B)_ OS automaticky priradí _zdrojový port_ pred odoslaním prvej správy.

- _Q:_ Aký je význam viacerých zdrojových IP adries nastavených na 1 rozhraní?
    + _A:_ IP adresy môžem použiť pre identifikáciu rôznych virtuálnych serverov (apache), ktoré bežia na 1 fyzickom zariadení

- __[!]__ použitím fcie `recvfrom()` načítam vždy len jednu správu => ak prečítam menej než je veľkosť správy, zvyšok správy sa _stratí_ => vždy sa pokúsim načítať toľko bajtov, koľko obsahuje najväčšia možná správa (aplikačné dáta bez hlavičiek protokolov).

#### Prevod ip adresy (text <-> číslo)

- použijem novší spôsob konverzie medzi textovým a číselným formátom IP adries cez fcie `inet_pton()` a `inet_ntop()` (v CV2 som použil zastaralý spôsob - `inet_aton()` a `inet_ntoa()`).
- textový (prezentačný) formát IPv4 a IPv6 adries ukladám do dostatočne dlhého poľa znakov:
    + `INET_ADDRSTRLEN` (16) a `INET6_ADDRSTRLEN` (46) => definované v `netinet/in.h`.
- číselný formát IP adries ukladám do `struct in_addr` (IPv4) a `struct in6_addr` (IPv6).

#### Prevod mien zariadení na ip adresy a názvov služieb na čísla portov

- DNS prináša možnosť identifikovať zariadenie nie len podľa IP adresy ale aj podľa mena (_hostname_). Vzhľadom na princíp _well-known_ portov, ktoré sú pridelené konkrétnym službám, môžem identifikovať službu nie len podľa čísla portu ale aj podľa jej názvu.
- program a dostupné fcie používajú _číselné_ formáty, ale človek si lepšie pamätá _textové_ formáty (názvy) => vykonám prevod mena zariadenia na IP adresu a názvu služby na číslo portu predtým, než ich použijem ako argumenty pri volaní fcií.
    + `getaddrinfo()` = vykoná prevod mien na čísla.
    + `getnameinfo()` = vykoná prevod čísel na mená.

##### Getaddrinfo

- `man getaddrinfo`.
- použijem identifikátor hosta (hostname alebo textový zápis IP adresy) a služby (meno alebo číslo portu) => získam adresu na (lineárne zreťazený) zoznam všetkých dvojíc _IP:PORT_, ktoré zodpovedajú zadaným vstupom.
- prvky zoznamu sú typu `struct addrinfo` a obsahujú všetky údaje potrebné pre vytvorenie socketu a jeho použitie (doména, typ, protokol, adresa).
    + pre detailnejšie zadanie požiadaviek použijem argument `hints` => konkrétna doména (IPv4 / IPv6) alebo typ socketu (TCP / UDP) => hodnota `0`, resp. `NULL` znamená "bez obmedzenia", flagy upravujú správanie fcie. Ak ako argument pre `hints` použijem `NULL` => fcia použije prednastavéné hodnoty (všetko bez obmedzenia).
- ak fcia zlyhá, chybovú správu získam cez `gai_strerror()`.
- __[!]__ po spracovaní získaných údajov musím upratať => `freeaddrinfo()`.

##### Getnameinfo

- `man getnameinfo`.
- získam zo `struct sockaddr` meno zariadenia a názov služby.
- pre výstupy vytvorím polia s dĺžkou `NI_MAXHOST` (1025) a `NI_MAXSERV` (32) => definované v `netdb.h`.
- detailné požiadavky na správanie fcie nastavím cez flagy (bitový vektor => nastavenie cez bitový OR).

## Rozširujúce úlohy

- **Úloha 1** - vytvorím program pre UDP klienta, ktorý bude posielať UDP správy s vlastným obsahom na zvolen IP adresu a port 1234.
    + v cykle načítavam riadok zo stdin a hneď odošlem v UDP správe.
- **Úloha 2** - vytvorím program pre UDP server, ktorý prijíma prichádzajúce správy na porte 1234.
    + prijaté správy a ich odosielateľa vypisujem na stdout v tvare _"[IP odosielateľa]:[PORT odosielateľa] - [SPRÁVA]"_.
- **Úloha 3** - vytvorím program, ktorý prijíma správy z multicastovej IP adresy 224.0.0.9 (_RIPv2_) => spracujem a vypíšem položky na stdout.
    + definujem hlavičku RIP správy.
    + __[!]__ všetky _viacbajtové_ hodnoty sú v _NetworkByteOrder_ => pred vypísaním prevediem na _HostByteOrder_.
