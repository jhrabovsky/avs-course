_[24-02-2017]_

# CVICENIE 2 - ARP

## TEORETICKA CAST - ARP

= popis struktury ARP hlavicky : [FORMAT ARP spravy](./arp_format.png) a [VYZNAM jedn poli](./arp-hdr-fields.jpg) => _RFC 826_ pre ARP (syntax, semantika a stavovy automat pre fungovanie ARP).  
= ARP => _CO_, _KEDY_, _AKO_ a _PRECO_?  
= teoreticky rozbor jednotlivych poli v hlavicke ARP => ich vyznam a mozne pouzitie pre implementaciu nastroja _arping_, ktory odosiela ARP ziadosti pre konkretnu IP a prijima (spracuje) prislusne odpovede.
    
**[!]** = deklaracie pre ARP v `net/if_arp.h` (prip. `linux/if_arp.h`) a `netinet/if_ether.h`, napr ARP hlavicka.  

## PRAKTICKA CAST - PROGRAM

**[!]** = pouzitie `char Payload[0]` => riesi problem pristupu k dalsej hlavicke:
  - _(A)_ **smernik** => zabera 4B, tj je navyse v danej strukture z pohladu prvkov hlavicky (~ vypln).  
  - _(B)_ pole s **0-velkostou** => ~ (A), ukazuje na _adresu konca struktury_, tj na nasledujucu hlavicku v sprave, ale nezabera nic, len pomocny _zapis adresy na prvy bajt za strukturov_.  

= _Q:_ Naco su v ARP hlavicke ziadosti uvedene SRC_MAC a SRC_IP?  
  => _A:_ Odstranenie spatneho ARP procesu (v opacnom smere), kedze pri generovani odpovede potrebuje povodny ciel MAC adresu povodneho zdroja => **[!]** _obojsmerne_ zistenie MAC adries bez nutnosti vykonania ARP procesu pre spatny smer.  

**[!]** = kontrolovat min velkost ramca => min _60B_ pre Ethernet => system automaticky doplni FCS (CRC).  

= `fprintf(stderr,...)` => chybovy vystup na STDERR.  
= pouzitie dynamickej pamate `malloc()` pre odosielanu ARP ziadost => nezabudnut na _uvolnenie pozicanej pamate_.  
**[!]** = vzdy _inicializovat_ vsetky premenne => zabezpecenie _ZNAMEJ_ nami urcenej pociatocnej hodnoty.  

= nacitanie MAC adresy z retazca => `sscanf()` => spec format textu, v ktorom je zapisana MAC adresa a hodnoty vlozit do jednotlivych poli MAC adresy v pamati => vhodne pre vyber poloziek z retazca => mozne pouzitie aj pre IP adresu.  
= pre jednotlive polozky MAC adresy => pouzitie _smernikovej aritmetiky_ => `*(srcMAC + i)` = adresa i-teho bajtu adresy (pocitame od 0) == `srcMAC[i]`.  

= _Q:_ ARP pre ine logicke/fyzicke adresy nez Ethernet a IP?  
  => _A:_ Uprava struktury a pouzitie len fiktivnej adresy na prvu z tychto adries v ARP hlavicke => napr ako pre RARP vo FrameRelay.  

### PREVOD IP ADRESY: TEXT<->CISLO

= IP adresa hosta ma dve formy: 
  - _(A)_ 32-bitove binarne cislo (network-byte-order),
  - _(B)_ text (bodkovy zapis - 4 dekadicke cisla z 0-255 oddelene bodkou).

= prevod medzi zapismi cez:   
  - `inet_ntoa()` a `inet_aton()` => **ZASTARALE**, podporuje len IPv4 (pouzite v programe na tomto cviceni).  
  - `inet_ntop()` a `inet_pton()` => **NOVE**, podporuju IPv4 aj IPv6 adresy (bude vyskusane v programe na dalsich cviceniach).  

= _kontrola spravnosti_ zadanej vstupnej IP adresy => overit oktetovy zapis (spoliehat sa na navratovu hodnotu vyssie uvedenych fcii) => pouzit prevody medzi ciselnym a textovym formatom.  

= _zachytenie odpovede_ na nasu ARP ziadost => sledovat prijate spravy a filtrovat/hladat _ocakavane/pozadovane_ polozky (v ARP hlavicke).  
  = citanie prijatych sprav => urcit velkost dolezitych dat a len tieto udaje precitat => alokacia pamate + cakanie na spravu (nekonecny cyklus).  

= vytvorenie pomocnej premennej na arp hlavicku v prijatom ramci => postupne porovnanie jednotlivych poloziek => 
  1. _opcode_ (ziadost/odpoved),
  2. _srcIP_ pre ziadost VS _targetIP_ (dstIP) pre odpoved.

**[!]** = `memcmp()` = porovnanie hodnot priamo v pamatiach => vhodne pre IP adresy (vzdy pevny pocet bitov).  
= pre vypis pozitivnej odpovede pouzit `printf()` s vhodnym formatovanim IP a MAC adries => **[!]** pouzitie * pri vkladani parametrov pre printf (pozaduje hodnotu).  
  => **[!]** pouzit _hh_ format => citanie len 1B namiesto 4B (plati pre _%x_) => pristup len k _1B_ pamate => ochrana pred pristupom do neznamej casti pamate mimo platny rozsah.  

**[!]** = pri generovani ARP ziadosti a kontrolovani ARP odpovede _SKONTROLOVAT_ spravnost zadanych parametrov:
  - `response->OPCODE == ARP_RESP` (je ARP odpoved),  
  - `response->SRC_IP == request->TARGET_IP` (odpoved prisla zo stanice, na kt som sa pytal v mojej ARP ziadosti).   

## VLAKNA

= pridanie `#include <pthread.h>` => pridanie fcii pre spravu vlakien.  
  = `man pthread_create` => vytvorenie a spustenie vlakna => pozriet typ pre startovaciu fciu vlakna.  
= _oddelenie_ generovania ARP ziadosti od sledovania prijatych ARP odpovedi => pouzit vlakna/procesy => _main_ je hlavne vlakno, kt vytvori dalsie vlakna.  
  = _main_ => odosielanie ARP ziadosti s oneskorenim (`sleep(1)` pre 1sec) v slucke => ochrana pred zahltenim siete.  
  = _vlakno_ => prijem ARP ziadosti, ich spracovanie a v pripade zhody vypis na obrazovku.  
   
= _Q:_ Ake su rozdiely medzi vlaknom a procesom?  
  => _A:_ popisat princip vlakna a procesu => porovnat => odlisna uroven zdielania pamate.  
  
= presunutie na _globalne premenne_ => zdielanie premennej medzi viacerymi vlaknami (napr socket).  
  = _kriticka sekcia_ => riesit pristup do premennej => semafor, mutex.    
= zvolit, kt premenne budu globalne => podla potrieb pristupu k premennym z roznych vlakien.  
**[!]** = _NEPOUZIVAT_ globalny pointer na lokalnu premennu, kt je def vo vnutri fcie => neobsahuje vzdy platnu hodnotu/adresu (napr. v kode *cv2_threads_arping.c* targetIP).  

= vytvorenie fcie, kt bude bezat vo vlakne => pevne definovany **funkcny prototyp** => `void * <nazov>(void * args)`.  

**[!]** = pri kompilacii pridat kniznicu pre vlakna => _libpthread_ => `-lpthread` (starsie kompilatory) alebo `-pthread` (novsie kompilatory).  
**[ECLIPSE]** = pridat _pthread_ kniznicu => _Project -> Properties -> C/C++ General -> Paths & Symbols -> Libraries_.  

## ROZSIRENE ULOHY

= **ULOHA 1** => oskenovat cez arping celu lokalnu siet => objavit aktivne sietove zariadenia v lokalnej sieti.  
= **ULOHA 2** => pridat _VLAN_ podporu => pridanie znacky (tagu) za SRC_MAC a pred Ethertype v ETH hlavicke.  
  - **VLAN_TAG** == _TAG_TYPE_ (0x8100 == 802.1Q, 16b), _PRI_ (3b), _DEI_(== CFI, 1b), _VLAN_ID_ (000 - FFF, 12b).  
  - pridanie _VLAN tagu_ do Ethernetovej hlavicky pred ethertype => TAG_TYPE = 802.1Q (_0x8100_).  
  - spec _PRI_ (3bit), _DEI_ (CFI = 1bit) a _VLAN-ID_ ako sucast jedneho 16bit cisla (2B) -> _[PRI, DEI, VLAN-ID]_.  
  
= **ULOHA 3** => _STP_ = generovanie STP ramcov s vhodnym obsahom.  
= **ULOHA 4** => _CDP_ = ohlasovat seba cez CDP ramce (alternativou je _LLDP_).
  
= **ULOHA 5** => implementacia _mostu_ (SW prepinac) => tema bude spracovana na dalsich cviceniach.  
  - nastavenie _promiskuitneho rezimu_ => prijem vsetkych ramcov bez ohladu na ich cielovu MAC adr.  
  - zameranie na prepinaciu tab => ukladanie, aktualizacia a vyhladavanie => volba vhodnej udajovej struktury (linearne zretazeny zoznam s operaciami - add, del a search).  
  - testovanie implementacie cez 2 VM (Bridge a Klient1) a nativny system (Klient2) cez premostenie prepojenia.  


