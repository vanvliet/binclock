# binclock

De klok is verdeeld in een denkbeeldig raster van 8 x 8, dus met een oppervlakte 64. In dat raster is de klok is verdeeld in 7 vakken met oppervlaktes van 1, 2, 4, 8, 16 of 32. Het donkergrijze vak in het midden telt niet mee. Merk op dat de oppervlaktes machten van 2 zijn, vandaar ‘binair’ in de naam. Elk getal tussen 0 en 63 kan op 1 unieke manier als som van 0, 1, 2, 3, 4, 5 of 6 van deze getallen worden geschreven. Voor de klok worden de getallen 0 t/m 23 voor de uren, en 0 t/m 59 voor de minuten gebruikt, als volgt:
- De uren worden bepaald door de oppervlakte van de rode + blauwe vakken. 
- De minuten worden bepaald door de oppervlakte van de gele + blauwe vakken.
- De witte vlakken (en het donkergrijze vak in het midden) doen NIET mee in de telling.

Op vliet.io draait een web versie van de klok. Omdat de kleurenklok niet makkelijk leest heb ik daar de tijd in tekst ernaast gezet. De tekstklok is een 12 uurs klok, de kleurenklok een 24 uurs klok.
