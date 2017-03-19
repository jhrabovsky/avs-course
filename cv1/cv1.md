_[20-02-2017]_

# CV1 - AVS

## ZDROJE

### LITERATURA

- [The Linux Programming Interface](http://man7.org/tlpi/)
- [Network Algorithmics](https://www.elsevier.com/books/network-algorithmics/varghese/978-0-12-088477-3)
- [Programming Linux Hacker Tools Uncovered](https://www.amazon.com/Programming-Linux-Hacker-Tools-Uncovered/dp/1931769613)

### MAN STRANKY 

- _packet_ = AF_PACKET.
- _socket_(7) = vseobecne info.
- _ip_, _raw_ = AF_INET.
  
- ak pripojim do kodu konkr _hlavickovy_ subor => cez `man <hdr_file>` ziskam popis suboru, tj zoznam a popis fcii a symbolov, kt tento hlavickovy subor deklaruje.

## UVODNE INFO O PREDMETE

- pripojenie ku lokalnej sieti specialne vytvorenej pre ucely AvS => praca len v tejto sieti => spolocne pripojenie pre celu skupinu.  
- WIFI parametre:   
    + AvS (SSID),
    + heslo (avstestnet),
    + WPA-personal (autentifikacia),
    + AES (sifrovaci algoritmus).
     
- _UVOD do predmetu_ => postup a zavislost od vedomosti v C => popisat pristup k studiu => priebeh cviceni, riesenie domacich uloh, odovzdavanie a ich hodnotenie.  
    - student bude v nepravidelnych intervaloch dostavat domace ulohy formou realizacie vlastneho programu s pozadovanymi fciami => pocet a typ uloh zavisi od fantazie vyucujuceho ako aj studentov (uprednostnovane pekne napady).  
- _PRIEBEH_ => len rozsirene CV bez prednasok => teoreticke casti vysvetlene pocas cvicenia (za behu).

- materialy pre predmet su umiestnene:  
    1. na [githube](https://github.com/jhrabovsky/AvS.git) (poslat email na jakub.hrabovsky@fri.uniza.sk so ziadostou o povolenie pristupu, sucasne uviest svoj ucet na github).
    2. na skolskej stranke vyucujuceho => [TU](http://www.kis.fri.uniza.sk/~hrabovsky/AvS).

## TEORETICKA CAST

- **CIELE** predmetu =>   
    1. komunikacia medzi procesmi (IPC) => (A) _vzdialena_ a (B) _lokalna_.
    2. implementacia _komunikacnych protokolov_ v Linuxe do vlastnych programov => prepojenie teorie s praktickou realizaciou (vidiet, ze naucena teoria skutocne funguje).
    3. navrh a implementacia udajovych struktur, kt su pouzivane v sietovych zariadeniach (prepinacia tab, smerovacia tab) a vplyv zvolenej udajovej struktury na vykon zariadenia.
  
### UVOD DO SOKETOV

- pristup ku sietovemu rozhraniu z programu (cez sokety) ~ rura = sposob ovladania => fcie a struktura (_RECV_ a _SEND_ buffre) soketu popisana na obrazku.  
- rozne komunikacne _domeny_ => AF_UNIX (AF_LOCAL), AF_INET, AF_INET6, AF_PACKET a ine.  
- rozne _typy_ soketov => SOCK_DGRAM, SOCK_STREAM, SOCK_RAW a ine.  
- rozne vlastnosti soketov (spravanie):  
    - poradie odoslanych a prijatych sprav (zavisi od transportneho protokolu), potvrdzovanie, spolahlivost, spojovanost, riadenie toku, riadenie zahltenia a ine.  
    - **[!]** _SOCK_STREAM_ (TCP) je potvrdzovany => ak druha strana neprijme/nepotvrdi segmenty, _musim cakat_ => _zaplnenie_ rury => _zapisujuci proces sa zastavi_ (je **blokovany**) => podobne aj pre citanie, ak druha strana este neodosiela => _synchronny_ pristup.  
    - _SOCK_DGRAM_ (UDP) je nepotvrdzovany => ak sa buffer zahlti (prijimajuca strana necita prijate spravy), dalsie datagramy su potichu _zahodene_.   

- _Q:_ Preco pouzit sokety pre _lokalnu_ komunikaciu?  
    - _A:_ **Dovod** : zjednotenie oboch komunikacii pod jednu implementaciu (_loopback_).  

- komunikacna **DOMENA** (namespace) = `PF_*` (stary) a `AF_*` (novy), napr AF_UNIX (AF_LOCAL), AF_INET, AF_INET6 (exist sposob, ako zjednotit implementaciu IPv6 a IPv4), AF_PACKET (pristup k celemu ramcu => vlastna tvorba struktury ramcov).  
- vyber domeny a typu soketu podla vlastnych _poziadaviek_ => k comu potrebujem pristup a co necham na OS => zavisi od aplikacie.  

- **TYP** soketu:  
    - `SOCK_STREAM` (spojovana spolahliva komunikacia => vyber konkr protokolu podla domeny) = **[!]** prenos vo forme _prudu bajtov_ (~ rura) => nezachovavaju sa hranice sprav pri citani/zapise => **[!] POZOR** = prijemca moze prijat data v inom pocte volani (read, recv) nez ich odoslal odosielatel (write, send) => **RIESENIE** : citanie v cykle, az kym som neprijal ocakavany/pozadovany pocet bajtov.     
    - `SOCK_DGRAM` = nespojovany => nesleduje reakcie z druhej strany (nepotvrdzovany => **[!]** nezastavi sa pri zaplneni, ale data prijate navyse su potichu _zahodene_), zachovava hranice medzi susednymi spravami.  
    - `SOCK_RAW` = nizsia uroven pristupu k hlavickam paketov => pracujem priamo s ramcom (zavisi od domeny).   
   
- **[!]** velkost prijmajuceho bafra nastavit na dostatocnu hodnotu, tj mat rezervu (radsej ovela viac nez max velkost spravy), ale POZOR na MTU, pre TCP je fragmentacia riesena automaticky (cez _Path MTU Discovery_), pre UDP zvolit mensiu velkost, aby fragmentacia nenastala (_512B_).  
  
- **[!]** pre SOCK_RAW pozadovane prava => pouzitie *CAP_NET_RAW* (v minulosti cez setUID a setGID) => `/sbin/getcap <program>` = schopnosti (capabilities), ktore ma program pri spusteni, napr /bin/ping.  
    - nastavenie schopnosti pre konkr program cez `/sbin/setcap`.   

### PRACA SO SOKETOM AF_PACKET - POSTUP

1. socket() = vytvorit soket.
2. bind() = pripojit adresu k soketu, tj prepojit soket s fyzickym rozhranim.
3. read(), write() = citanie (prijem) a zapis (odosielanie).
4. close(), shutdown() = zatvorenie soketu.    

- pouzitie man stranok pre jednotlive prikazy.
- `man socket` = popis sys volania `socket()` => parametre => *protocol* pre SOCK_RAW == **ETH_TYPE** (`man 7 packet`).    
  
- **[!]** po ukonceni programu sa nezdielane zdroje automaticky zatvaraju, ALE sietovy proces je rieseny ako daemon, tj bezi stale !!! => nevyhnutne pravidelne upratovat, tj riesit _pravidelne uvolnovanie zdrojov_.  

### PORADIE BAJTOV

- **[!]** (A) _LITTLE-ENDIAN_ [LE] a (B) _BIG-ENDIAN_ [BE] => urcuju usporiadanie bajtov vo _viacbajtovych_ premennych.  
    - (A) ulozenie nizsich bajtov premennej na _nizsie adresy_ v pamati, (B) ulozenie nizsich bajtov premennej na _vyssie adresy_ v pamati => rozne **CPU** pouzivaju rozne poradie bajtov => Intel je LE ale PowerPC (v prepinacoch) je BE.
    - zjednotenie pre rozne systemy z pohladu _sietovej komunikacie_ => **Network-byte order** (v sieti pocas prenosu) a **Host-byte order** (na koncovej stanici).
  
- konverzia (_arpa/inet.h_) : `ntohs()`, `ntohl()`, `htons()`, `htonl()` => s=short (_short_), l=long (_int_).  
- odosielanie a prijem retazcov => identifikacia konca (cez _'\\0'_). **POZOR!** na vypocet velkosti retazcov (+-1), `strlen()` nezapocitava ukoncovaci znak.  

### ZAROVNANIE A VYPLN

- _struktury_ => kazdy prvok prevedeny medzi Host-byte-order a Network-byte-order samostatne (uloha programatora) => sirka datovej zbernice je ale 32/64 bitova => musime vzdy nacitat celu sirku => udaje mensie nez sirka su nasledne vybrane.  

- **[!!]** rozdelenie struktury na prvky s roznou sirkou vedie k neefektivnemu pristupu k niektorym prvkom struktury => optimalizacia kompilatorov sposobi posunutie prvkov struktury kvoli lepsiemu pristupu k prvkom z pohladu CPU => _zvacsenie_ velkosti struktury posunutim prvkov a vlozenim vyplne v pamati.  
    - **[!]** odosleme viac dat (vratane vyplne) => **PROBLEM** pri vzdialenej komunikacii medzi roznymi zariadeniami.  
    - pouzitie `__attribute__((packed));` hned za deklaraciou struktury => _zakazanie_ optimalizacie a pridavania vyplne.  

- **[!]** prenos struktury cez siet => vnimana ako suvisly blok pamate (pole bajtov), kt pozostava z viacerych prvkov rozneho typu.   

## PRAKTICKA CAST = PROGRAM

- nastavenie dialektu pre _ECLIPSE_ => `--std=gnu99` (Project -> Properties -> C/C++ Build -> Settings -> Dialect).  
- pre zdielanie adresara medzi VM a nativnym systemom => pridat pouzivatela vo VM (student) do skupiny _vboxsf_ cez `usermod -a -G vboxsf student`.  

### ZADANIE

- Pozadovane funkcie programu :  
    - viacnasobne odoslanie spravy (ramca) do siete => obsah ramca (text v payloade) zvoli student.  

- pouzivanie manualovych stranok v linuxe => `man <sekcia> <stranka>` (spomenut `-k` a `-f`) => struktura manualovej stranky (SYNOPSIS, RETURN VALUE, ERRORS).  
- **[!]** AF_PACKET -> SOCK_RAW, *protocol* = ETH_TYPE z 802.3 v _Network-byte-order_.  
    - overenie spravneho vykonania funkcie vzdy cez manualovu stranku => **RETURN_VALUE** a **ERRORS** => pouzitie globalnej premennej *errno* => **POZOR** na zmenu errno = vzdy zobrazit hned za vznikom chyby, aby nebola medzitym zmenena inou fciou.  

### AF_PACKET

- struktura adresy (`struct sockaddr_ll`) => popis prvkov (`man 7 packet`).
- odovzdavanie premennych typu `struct` do fcii casto cez **smernik**.
- rozne typy struktur pre adresu v `bind()` => pretypovanie z dovodov _spatnej kompatibility_ => `struct sockaddr` je len genericka.
- praca s jednotlivymi typmi adries podla domeny soketu => ~ dedicnost v OOP (rozny pohlad na strukturu / jej format).
- `sll_ifindex` = index rozhrania, kt ziskam cez `if_nametoindex()` => identifikacia rozhrania podla jeho mena.  
- **[!]** vzdy najprv inicializovat (vynulovat) kazdu premennu => pouzit `memset()`.  

### STRUKTURA HLAVICKY

- navrh a deklaracia ramca a jeho hlaviciek => navrhnut vhodnu strukturu, kt predstavuje hlavicku ako zoskupenie udajov v spravnom poradi => podla formatu hlavicky v RFC.  

- **[!]** _zdrojova MAC adresa_ => pre pokusy pouzit _privatnu MAC adresu_ => "xxxxxxx1x" pre najvyssi bajt MAC adresy => nastavit na _0x02_.  

- **[!]** zopakovat:
    - zapis cisel v roznych formatoch v C => 10-/8-/16-sustava.
    - kopirovanie retazcov v C => `strncpy()`, `memcpy()`.

### KOMPILACIA A ANALYZA PROGRAMU

- **[!]** kompilacia vzdy s prepinacom `-Wall` => nechceme vo vysledku ziadny warning ani error.
- `gcc -E` = len predkompiluje bez linkovania => pozriet, ako su makra chapane a ako vyzera `struct sockaddr`.
- `strace` = pouzitie pre sledovanie systemovych volani pre program.
- `mtrace` = overit spravne uvolnovanie pamate => detekcia _memory leaks_.

## ROZSIRUJUCE ULOHY

- **ULOHA 1** => ako sa zmeni sprava, ked zapiseme _ETH_TYPE_ v _Little-Endian_ => analyzovat vo Wiresharku.  
- **ULOHA 2** => postupne generovanie zdrojovych MAC adries v cykle s krokom _+1_.  
    - vyuzitie vhodneho pretypovania len casti pola => pouzit iny pohlad na danu cast (char pole[4] => int cislo).  
- **ULOHA 3** => napisat *arping* => vytvorit ARP ziadost, odoslat ju a cakat na odpoved.  
