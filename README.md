# mp-projekt2
We have used python for the gui and C for the backend

# Åbning af GUI

    python3 gui.py

## Kortkommandoer

Startup-fasen:

LD <filename>

    Loader et deck fra en fil. Hvis ingen fil angives, oprettes et standarddeck.

SW

    Viser decket.

SI <split>

    Laver et split shuffle.

SR

    Laver et random shuffle.

SD <filename>

    Gemmer det aktuelle deck til en fil.

P

    Starter spillet og går over i play-fasen.

QQ

    Afslutter programmet.

Play-fasen:

    <from>-><to>


    Flytter kort mellem kolonner og foundations.

Eksempler:

    C1:3D->C4

    C7:AS->F1

    F1->C3

Q

    Afslutter det nuværende spil og går tilbage til startup-fasen.