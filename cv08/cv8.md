---
title: "AVS - CV8: TCP"
author: [Jakub Hrabovský, Martin Kontšek]
date: "2017-04-09"
...

# AVS - CV8: TCP

## Teoretická časť - Vlastnosti TCP

- témou cvičenia je __TCP__ => použijem socket typu `SOCK_STREAM`.
- TCP poskytuje komunikáciu medzi dvojicou zariadení:

    + __spojovaná__ - pred začatím prenosu dát vytvorím __reláciu__ medzi komunikujúcimi stranami (point-to-point).
    + __spoľahlivá__ - chybne prenesené a stratené segmenty sú opätovne odoslané => zabezpečuje mechanizmus _potvrdzovania_.
    + __prúdovo-orientovaná__ - aplikačné údaje sú rozdelené do segmentov bez ich významu v aplikácii => neexistujú jasné hranice medzi údajmi, tj. dáta môžem odoslať cez _N_ volaní niektorej z fcií zápisu (`write()`, `send()`) a prijať na cieľovej stanici cez _M_ volaní niektorej z fcií čítania (`read()`, `recv()`).
    + poskytuje __riadenie toku__ (predchádzanie zahlteniu _prijímajúcej strany_) a __riadenie zahltenia__ (predchádzanie zahlteniu _siete_).

- pred odoslaním musím vytvoriť spojenie s druhou stranou (__3-way-handshake__) a po skončení prenosu zase uzatvorím spojenie (__2-way-handshake__).

## Praktická časť - Program

### Server

1. vytvorím socket z domény `AF_INET` typu `SOCK_STREAM` s protokolom `0` => `socket()`.
2. vyplním adresu socketu a povolím `SO_REUSEADDR` pre socket => `setsockopt()`.
3. prepojím socket s adresou => `bind()`.
4. prepnem socket do pasivného režimu a nastavím maximálny počet čakajúcich spojení (__BACKLOG__) => `listen()`.
5. v slučke prijímam nové spojenia a vytváram aktívny socket => `accept()`.
6. komunikujem s druhou stranou cez aktívny socket => `read()`, `write()`, `recv()`, `send()`.
7. ukončím aktuálne spojenie (zatvorím aktívny socket) a pokračujem krokom 5.
8. ak už neočakávam nové spojenia, zatvorím pasívny socket => `close()`, `shutdown()`.

### Klient

1. vytvorím socket z domény `AF_INET` typu `SOCK_STREAM` s protokolom `0` => `socket()`.
2. vyplním adresu vzdialenej strany, s ktorou chcem komunikovať.
3. požiadám o vytvorenie spojenia s druhou stranou => `connect()`.
4. komunikujem s druhou stranou => `read()`, `write()`, `recv()`, `send()`.
5. ukončím spojenie, tj. zatvorím socket => `close()`, `shutdown()`.

### Obsluha klientov

- _Q:_ AKO obslúžim viacerých klientov?
    + (A) __iteratívny prístup__ - vždy obslúžim __len jedného__ klienta v danom okamihu,
    + (B) __konkurenčný prístup__ - obslúžim viacero klientov súčasne => pre každé prichádzajúce spojenie vytvorím samostatný výkonný blok:
        1. potomkový proces => spôsobí duplikáciu popisovačov => rodičovský proces automaticky uzatvára klientský (aktívny) socket.
        2. vlákno.

- __[!]__ každý potomkový proces alebo vlákno musím "pochovať", keď tento výkonný blok skončí svoju prácu (pošle signál `SIGCHLD`) => prečítam návratovú hodnotu, aby systém mohol spokojne zrušiť pamäť pridelenú tomuto bloku => `wait()`.
    + signály v OS predstavujú __softvérové prerušenie__ => ak chcem použiť vlastnú fciu ako obslúžny program (handler) pre konkrétny signál, registrujem fciu cez `sigaction()` => získam aktuálne nastavenia pre dané prerušenie (načítam aktuálny stav), zmením len obslúžny program na moju fciu a opäť nastavím.
    + ak proces prijíme signál, preruší aktuálnu činnosť a skočí na obslúžny program. Po ukončení obsluhy sa vráti späť na pôvodné miesto => prerušenie spôsobí ukončenie aktuálne bežiacej fcie s chybou __EINTR__ => ak proces prijíme prerušenie v momente, kedy čakám na nové spojenie (`accept()`), fcia skončí s chybou a ja ju musím opäť zavolať, tj. pokračujem ďalšou iteráciou v nekonečnej slučke (`continue`).

- ukončenie spojenia druhou stranou zistím podľa:
    + ak čítam zo socketu => `read()` vráti 0,
    + ak zapisujem do socketu => `write()` vráti chybu `EPIPE` alebo odošle signál prerušenia `SIGPIPE` => použijem flag `MSG_NOSIGNAL` pri volaní `send()`, aby som zabránil vygenerovaniu prerušenia.

#### Time_wait

__ZDROJ__ = [TU](http://www.serverframework.com/asynchronousevents/2011/01/time-wait-and-its-design-implications-for-protocols-and-scalable-servers.html)

![Stavy pre ukončenie TCP spojenia](./time-wait.png)

- proces vykoná __aktívne uzavretie (active close)__, keď ako prvý odošle __FIN__ => aj keď tento proces následne skončí, systém musí počkať na __ACK__ a __FIN__ odoslaný druhou stranou spojenia, aby sa presvedčil o korektnom ukončení TCP spojenia (__FIN_WAIT1__ a __FIN_WAIT2__) => systém v mene procesu odošle záverečný __ACK__, aby potvrdil prijatý FIN príznak => proces s aktívnym uzavretím spojenia čaká určitú dobu (zostane v stave __TIME_WAIT__), kým druhá strana nepríjme ACK a TCP spojenie sa tak korektne uzavrie.

- po ukočení spojenia jednou stranou je adresa spojenia `[IP]:[PORT]` ešte po určitú dobu zablokovaná (v stave __TIME_WAIT__), tj. nemôžem použiť adresu pre nový socket (vznikne chyba `EADDRINUSE`) =>
    + pre klienta nepodstatné, lebo používa dynamické porty a teda pravdepodobnosť znovupoužitia rovnakého portu je minimálna,
    + pre server dôležité, keďže server počúva vždy na jednom konkrétnom porte => nastavím voľbu socketu `SO_REUSEADDR` po vytvorení socketu a pred prepojením socketu s adresou (`setsockopt(fd, SOL_PACKET, SO_REUSEADDR, &optval, optval_len)`).

-----

- TCP/IP stack implementovaný v jadre posúva údaje z aplikačnej vrstvy do buffra => keď buffer obsahuje dostatočné množstvo bajtov, systém vytiahne z buffra všetky údaje, zabalí ich do segmentu a odošle => ak moja aplikácia vyžaduje okamžité odosielanie údajov, musím nastaviť v TCP hlavičke príznak _PSH_.

- __[!]__ odosielanie aj príjem dát vykonávam vždy __v cykle__, kým nespracujem všetky dostupné údaje => dôvodom je prúdové spracovanie TCP.
- spojenie ukončím cez:
    + `shutdown()` ponúka pokročilejšie ukončenie spojenia => __[!]__ spojenie sa ukončí okamžite bezohľadu na počet popisovačov, ktoré ukazujú na spojenie (napríklad kopírovanie popisovačov pri vytvorení potomkového procesu).
    + `close()` spôsobí ukončenie spojenia, až keď sú zatvorené všetky popisovače ukazujúce na spojenie.

- `getpeername()` - opýtam sa na adresu (`struct sockaddr`) druhej strany spojenia.
- `getsockname()` - opýtam sa na lokálnu adresu, ktorá je pridelená socketu.

- __[!]__ pre otestovanie spracovania viacerých klientov súčasne cez potomkové procesy použijem `sleep()`, aby som predĺžil dobu obsluhy.
- ak chcem odoslať RST, nastavím socketu voľbu `SO_LINGER` s timeoutom `0` a následne zatvorím socket cez `close()`.

## Úlohy na cvičenie

- vytvorím server, ktorý čaká na žiadosť o TCP spojenie od klienta.
    + server prečíta riadok ukončený znakom nového riadku `\n` a vypíše ho spolu s údajmi o klientovi na terminál => "[IP adresa klienta : Port klienta] : [prijatá správa]".
- vytvorím klienta, ktorý požiada o založenie TCP spojenia, prečíta riadok načítaný z terminálu a odošle ho na server.
- upravím server tak, aby pre každého klienta vytvoril potomkový proces => proces obslúži klienta a skončí.
