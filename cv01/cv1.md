_[17-02-2018]_

# CV1 - AVS

## ZDROJE

### LITERATÚRA

- [The Linux Programming Interface](http://man7.org/tlpi/)
- [Network Algorithmics](https://www.elsevier.com/books/network-algorithmics/varghese/978-0-12-088477-3)
- [Programming Linux Hacker Tools Uncovered](https://www.amazon.com/Programming-Linux-Hacker-Tools-Uncovered/dp/1931769613)

### MANUÁLOVÉ STRÁNKY 

- `man packet` = AF_PACKET.
- `man 7 socket` = všeobecné informácie ku soketom.
- `man ip`, `man raw` = AF_INET.
- `man <hlavičkový súbor>` = získam popis hlavičkového súboru => zoznam a popis funkcií a symbolov, ktoré hlavičkový súbor deklaruje.  

## ÚVODNÉ INFORMÁCIE O PREDMETE

- predmet vyžaduje základné znalosti jazyka C.
- vyučovanie predmetu AVS sa primárne zameriava na praktické oboznámenie sa so vzdialenou komunikáciou medzi procesmi cez počítačovú sieť => počas cvičenia je vždy stručne vysvetlená základná teória k danej látke, ktorá je následne prakticky precvičovaná na zvolených úlohách.
- na konci každej hodiny sú uvedené odporúčané domáce úlohy pre upevnenie a prípadné rozšírenie učiva. 
- materiály pre predmet sú umiestnené na [githube](https://github.com/jhrabovsky/AvS.git).

## TEORETICKÁ ČASŤ

- **CIELE** predmetu:   
    1. komunikácia medzi procesmi (IPC): (A) _vzdialena_ a (B) _lokalna_.
    2. _komunikačné protokoly_ v OS Linux a ich použitie vo vlastných programoch => prepojenie teórie TCP/IP s reálnym nasadením.
    3. návrh a implementácia údajových štruktúr, ktoré sú používané v sieťových zariadeniach (prepínacia a smerovacia tabuľka) a vplyv zvolenej údajovej štruktúry na výkon zariadenia.
  
### ÚVOD DO SOKETOV

- prístup k sieťovému rozhraniu z programu (cez sokety) => manipulácia a spôsob ovládania sú veľmi podobné rúram => funkcie a štruktúra soketu (_RECV_ a _SEND_ vyrovnávacie pamäte - buffre).  
- rôzne komunikačné _domény_: `AF_UNIX` (`AF_LOCAL`), `AF_INET`, `AF_INET6`, `AF_PACKET` a iné.  
- rôzne _typy_ soketov => `SOCK_DGRAM`, `SOCK_STREAM`, `SOCK_RAW` a iné.  
- rôzne vlastnosti soketov (správanie):  
    * poradie odoslaných a prijatých správ (závisí od transportného protokolu), potvrdzovanie, spoľahlivosť, spojovanosť, riadenie toku, riadenie zahltenia a iné.  
    * [!] `SOCK_STREAM` (TCP) je potvrdzovaný => ak druhá strana nepríjme/nepotvrdí segmenty, _musím čakať_ => v krajnom prípade dôjde k _zaplneniu_ rúry => _zapisujúci proces sa zastaví_ (je **blokovaný**). Podobne platí aj pre čítanie - ak druhá strana ešte neodosiela => _synchrónny_ prístup.  
    * `SOCK_DGRAM` (UDP) je nepotvrdzovaný => ak sa vyrovnávacia pamäť zahltí (prijímajúca strana nečíta prijaté správy) => [!] ďalšie datagramy sú potichu _zahodené_.   

- Q: PREČO mám použiť sokety pre _lokálnu_ komunikáciu?
    * A: Zjednotenie oboch typov komunikácie pod jednu implementáciu (cez _loopback_).  

- komunikačná **DOMÉNA** (_namespace_) = `PF_*` (zastaralý zápis) a `AF_*` (odporúčaný zápis), napr. `AF_UNIX` (`AF_LOCAL`), `AF_INET`, `AF_INET6`, `AF_PACKET` (poskytuje prístup k celému rámcu vrátane všetkých hlavičiek => tvorba vlastnej štruktúry rámcov).  
- výber domény a typu soketu podľa _požiadaviek_ svojej aplikácie => k čomu potrebujem prístup v rámci prijatých správ a čo nechám na OS.  

- **TYP** soketu:  
    * `SOCK_STREAM` (spojovaná spoľahlivá komunikácia => výber konkrétneho protokolu podľa domény) = [!] prenos vo forme _prúdu bajtov_ (~ rúra) => nezachovávajú sa hranice medzi susednými správami pri čítaní a zápise.
        + **POZOR**: príjemca môže prijať dáta v inom počte volaní (`read()`, `recv()`) než ich odoslal odosielateľ (`write()`, `send()`) => **RIEŠENIE** : čítanie v cykle, až kýmm som neprijal očakávaný (požadovaný) počet bajtov.     
    * `SOCK_DGRAM` (nespojovaný, nespoľahlivý) = nesleduje reakcie z druhej strany, (nepotvrdzovaný) = nezastaví sa pri zaplnení, ale dáta prijaté navyše potichu _zahadzuje_, zachováva hranice medzi susednými správami.  
    * `SOCK_RAW` = nižšia úroveň prístupu k hlavičkám paketov => pracujem priamo s rámcom (závisí od domény).   
   
- [!] veľkosť prijímajúcej pamäte nastavím na dostatočnú hodnotu tak, aby som mal rezervu (radšej oveľa viac než je maximálna velkosť správy), ale POZOR na MTU => pre TCP je fragmentácia riešená automaticky (cez _Path MTU Discovery_), pre UDP musím zvoliť menšiu veľkosť, aby fragmentácia nenastala (_512B_).  
  
- [!] `SOCK_RAW` požaduje špecifické práva => použitie `CAP_NET_RAW` (v minulosti cez `setUID` a `setGID`) => `/sbin/getcap <program>` = schopnosti (capabilities), ktoré má program pri spustení, napr. /bin/ping.  
    - nastavenie schopností pre konkrétny program cez `/sbin/setcap`.   

### PRÁCA SO SOKETOM AF_PACKET - POSTUP

1. `socket()` = vytvorím soket.
2. `bind()` = naviažem adresu k soketu, tj previažem soket s fyzickým rozhraním.
3. `read()`, `write()` = čítam (príjem) a zapisujem (odosielanie) dáta.
4. `close()`, `shutdown()` = zatvorím soket.    
  
- [!] po ukončení programu sa nezdieľané zdroje automaticky zatvárajú, ale sieťový proces je v praxi riešený ako _daemon_, tj beží stále (bez ukončenia) => nevyhnutné pravideľné "upratovanie" (_pravidelne uvolňovanie zdrojov_).  

### PORADIE BAJTOV

- [!] (A) _LITTLE-ENDIAN_ [LE] a (B) _BIG-ENDIAN_ [BE] => určuju usporiadanie bajtov vo _viacbajtových_ premenných.  
    * (A) uloženie nižších bajtov premennej na _nižšie adresy_ v pamäti
    * (B) uloženie nižších bajtov premennej na _vyššie adresy_ v pamäti 
    * => rôzne **CPU** používajú rzzne poradie bajtov => Intel je _LE_ ale PowerPC (v prepínačoch) je _BE_.
    * zjednotenie pre rôzne systémy z pohľadu _sieťovej komunikácie_: **Network-byte order** (v sieti počas prenosu) a **Host-byte order** (na koncovej stanici).
  
- konverzia (`arpa/inet.h`) : `ntohs()`, `ntohl()`, `htons()`, `htonl()`, kde _s_=short (_short_), _l_=long (_int_).  
- odosielanie a príjem reťazcov vyžaduje identifikáciu konca (cez `\0`). POZOR na výpočet veľkosti reťazcov (+-1), `strlen()` nezapočítavá ukončovací NULL znak.  

### ZAROVNANIE A VÝPLŇ

- `struct` = každý prvok musí byť prevedený medzi `Host-byte-order` a `Network-byte-order` samostatne (úloha programátora) => šírka dátovej zbernice CPU je ale pevná (32b / 64b) => musím vždy načítať celú šírku z pamäte => údaje menšie než táto šírka sú následne vybrané.  

- [!] rozdelenie štruktúry na prvky s rôznou šírkou vedie k neefektívnemu prístupu k niektorým prvkom štruktúry zo strany CPU => optimalizácia kompilátorov spôsobí posunutie týchto prvkov kvôli lepšiemu (efektívnejšiemu) prístupu k nim => _zväčšenie_ (_zarovnanie_) štruktúry posunutím prvkov a vložením *výplne* v pamäti.  
    * **PROBLÉM**: pri vzdialenej komunikácii medzi rôznymi zariadeniami odošlem viac dát (vrátane výplne) než druhá strana očakáva (podľa protokolov, kde špecifikovaná pevná štruktúra hlavičiek).  
    * **RIEŠENIE**: použijem `__attribute__((packed));` hneď za deklaráciou štruktúry => _zakázanie_ optimalizácie a pridávania výplne. 

## PRAKTICKÁ ČASŤ - PROGRAM

- pre praktickú časť vyučovania je predpripravený súbor virtuálneho zariadenia (`AVS-64-IDE.ova`), ktorý obsahuje prostredie s nástrojmi požadovanými pre výučbu.
    * súbor poskytne vyučujúci na cvičení.
    * po importovaní preinštalujte (aktualizujte) `Guest Additions` na verziu kompatibilnú s Vašou verziou nástroja VirtualBox. 
- nastavenie dialektu pre _ECLIPSE_ => `--std=gnu99` (Project -> Properties -> C/C++ Build -> Settings -> Dialect).  
- pre zdieľanie adresára medzi VM a natívnym systémom musím pridať používateľa vo VM (_student_) do skupiny `vboxsf` cez `usermod -a -G vboxsf student`.  

### ZADANIE

- Požadované funkcie programu :  
    * viacnásobné odoslanie správy (rámca) do siete s ľubovoľným obsahom za Ethernetovou hlavičkou.  

- používanie manuálových stránok v OS Linux => `man <sekcia> <stranka>` (vhodné prepínače sú `-k` a `-f`) => štruktúra manuálovej stránky (`SYNOPSIS`, `RETURN VALUE`, `ERRORS`, ...).  
- [!] `AF_PACKET` -> `SOCK_RAW`, _protocol_ = `ETH_TYPE` z 802.3 v _Network-byte-order_.  
    * úspech funkcie vždy overím cez návratovú hodnotu (popis uvedený v manualej stránke pre zvolenú funkciu) => `RETURN_VALUE` a `ERRORS` => mnohé systémové funkcie používajú globálnu premennú `errno` (vyžaduje `#include <errno.h>`) => **POZOR** na zmenu `errno` volaním inej funkcie => vždy overím hneď za vznikom chyby, aby hodnota `errno` nebola medzitým zmenená inou funkciou.  

### AF_PACKET

- štruktúra adresy (`struct sockaddr_ll`) popísaná v `man 7 packet`.
- odovzdavanie premenných typu `struct` do funkcií často cez **smerník**.
- rôzne typy štruktúr pre adresu v `bind()` => pretypovanie z dôvodov _spätnej kompatibility_ => `struct sockaddr` je len generická.
- práca s jednotlivými typmi adries podľa domény soketu => podobný princíp ako dedičnosť v OOP (rôzny pohľad na štruktúru a jej formát).
- `sll_ifindex` = index rozhrania, ktorý získam cez `if_nametoindex()` => identifikácia rozhrania podľa jeho mena.  
- [!] každú premennu vždy najprv inicializujem (vynulujem) pred ďalšim použitím, napr. cez `memset()`.  

### ŠTRUKTÚRA HLAVIČKY

- návrh a deklarácia rámca a jeho hlavičiek => navrhnem vhodnú štruktúru, ktorá reprezentuje hlavičku ako zoskupenie údajov v požadovanom poradí (podľa formátu hlavičky v RFC dokumentoch).  

- [!] _zdrojová MAC adresa_ = pre pokusy použijem _privátnu MAC adresu_ ("xxxxxxx1x" pre najvyšší bajt MAC adresy) => nastavím na _0x02_.  

- [!] zopakujem:
    - zápis čísel v rôznych formátoch v C => 10-/8-/16-sústava.
    - kopírovanie reťazcov v C => `strncpy()`, `memcpy()`.

### KOMPILÁCIA A ANALÝZA PROGRAMU

- [!] kompilácia vždy s prepínačom `-Wall` => nechcem vo výsledku žiadny `warning` ani `error`.
- `gcc -E` = len predkompiluje bez linkovania => pozriem, ako sú makra preložené a ako vyzerá `struct sockaddr`.
- `strace` = použijem pre sledovanie systémových volaní programu.
- `mtrace` = overím správne uvolňovanie pamäte => detekcia _memory leaks_.

## ROZŠIRUJÚCE ÚLOHY

- **ÚLOHA 1** = ako sa zmení správa, keď zapíšem `ETH_TYPE` v _Little-Endian_ => analyzujem v nástroji Wireshark.  
- **ÚLOHA 2** = postupne generujem správy s narastajúcou zdrojovou MAC adresou v cykle s krokom _+1_.  
    * využitie vhodného pretypovania len časti poľa so zdrojovou MAC adresou => použijem iný pohľad na danú časť (char pole[4] => int číslo).  
- **ÚLOHA 3** = napíšem *arping* => vytvorím ARP žiadosť, odošlem ju a čakám na odpoveď.  
