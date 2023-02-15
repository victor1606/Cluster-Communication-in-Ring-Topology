Calugaritoiu Ion-Victor

Algoritmi Paraleli si Distribuiti

Task 1:
 - Stabilirea topologiei:
    - Fiecare coordonator isi trimite rank-ul catre fiecare worker, astfel
    acestia afland rank-ul parintelui;
    - Fiecare coordonator isi trimite matricea topologiei mai departe catre
    "next_coordinator", intr-o ordine ascendenta a rank-urilor (0->1; 1->2; 
    2->3; 3->0); - ordine clockwise pe inel;
    - Fiecare coordonator primeste matricea de la "previous_coordinator", 
    intr-o ordine descendenta a rank-urilor (3->2; 2->1; 1->0; 0->3;); - 
    ordine counter-clockwise pe inel;
    - Sunt folosite matrici auxiliare "dummy", care sunt replici ale matricei
    principale a topologiei, numita "workers";
    - Cand se face un receive, datele primite sunt puse in "dummy", iar apoi
    comparate cu cele din "workers": daca un camp din "workers" este 0 
    (nepopulat inca), iar in matricea nou primita este populat, se copiaza 
    campul din matricea noua in cea principala; astfel, se evita suprascrierea
    datelor din matricea topologiei;
    - Operatiile anterioare de send si receive sunt repetate de 3 ori, pentru
    a propaga topologia finala catre fiecare coordonator (inelul este
    parcurs ascendent);
    - Dupa ce toti coordonatorii detin topologia completa, fiecare si-o trimite
    catre fiecare copil;

Task 2:
 - Realizarea calculelor:
    - Coordonatorul 0 initializeaza vectorul si il trimite catre
    "next_coordinate";
    - Cand toti coordonatorii au primit vectorul, si-l trimit catre workeri;

    - Fiecare worker inmulteste cu 5 doar campurile care ii sunt rezervate,
    in functie de rank, numarul total de workeri si dimensiunea vectorului;
    - Numarul de calcule per worker: marimea vectorului / numarul de workeri;
    - Indexul in vector de la care porneste calculele: rank-ul worker-ului *
    numarul de calcule per worker;
    - Ultimul worker se ocupa de calculele ramase (cantitate neglijabila), in
    cazul in care numarul de calcule per worker nu rezulta un numar intreg; 
    - Fiecare worker trimite vectorul modificat spre parinte;

    - Dupa ce coordonatorii au primit vectorul de la copii, il trimit mai 
    departe catre "next coordinator";
    - Cand vectorul modificat complet ajunge la coordonatorul 0, este afisat.

Task 3:
 - Tratarea defectelor pe canalul de comunicatie (0-1):
    - Spre deosebire de task-ul 1, nu se mai pot efectua comunicatii directe
    intre 0 si 1, deci inelul nu poate fi parcurs circular ascendent;
    - Pentru a propaga matricea topologiei, se fac urmatoarele operatii:
        - Se propaga matricea lui 1 ascendent de la 1->2->3->0;
        - In acest punct, doar coordonatorul 0 are topologia completa;
        - Se propaga matricea lui 1 descendent de la 0->3->2->1;
        - Toti coordonatorii au topologia completa;
    
    - Pentru realizarea calculelor se urmareste exact acelasi model ca la 
    task 1, insa pentru propagarea vectorului se utilizeaza modelul prezentat
    anterior la propagarea matricei.

Bonus:
 - Tratarea partitionarilor:
    - Coordonatorii 0, 2, 3 se alfa in alta topologie fata de 1;

    Pentru a propaga matricea topologiei cu 0, 2, 3 se foloseste aceeasi 
    abordare ca la task 3:
        - Se propaga matricea lui 0 descendent de la 0->3->2;
        - In acest punct, doar coordonatorul 2 are topologia completa;
        - Se propaga matricea lui 2 ascendent de la 2->3->0;
        - Toti coordonatorii au topologia completa;
    
    Pentru topologia coordonatorului 1, nu exista vecini la care sa se trimita
    date, deci se vor trimite doar catre workerii acesteia.

    Pentru realizarea calculelor, vectorii sunt trimisi analog modului in care
    sunt trimise matricile de topologie. Workerii coordonatorului 1 nu
    participa la efectuarea calculelor din vector, deci numarul total de
    workeri scade, si numarul de calcule per worker creste. Indecsii de la care
    pornesc workerii sunt de asemenea calculati diferit, tinand cont de lipsa
    celor din clusterul deconectat.
