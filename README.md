
# Projet de programmation système

## A propos

### Sujet

Simuler l'environnement d'un immeuble d'habitation et/ou de bureau, à l'aide des outils systèmes tels que les processus, threads, objets IPC, etc...

### Contexte

* Notre immeuble comporte 3 ascenseurs qui opèrent de concert et couvrent 25 étages.

* Chaque ascenseur dispose d'une capacité limitée, et possède 3 états : *en marche*, *à l'arrêt à un étage*, et *en veille*. S'il tombe en panne, un technicien est dépéché sur place.

* Les visiteurs/livreurs peuvent contacter les résidents de l'immeuble et demander l'accès. Une IA s'occupe ensuite de les répartir dans les différents ascenseurs.

* Les résidents peuvent également se déplacer, quitter/entrer dans l'immeuble, cependant ils n'ont pas besoin d'autorisation pour le faire.

## Choix de conception

1. Processus Immeuble
	* Fork pour créer les 3 ascenseurs
	* Instancie la mémoire partagée *DwellerList*
	* Instancie la file de message *Visiteur-Résident*
	* Affiche la *DwellerList* si reception d'un signal SIGUSR1
	* Détruit les objets IPC à la réception d'un signal SIGINT
	
2. Processus Ascenseur
	* Créé la mémoire partagée *WaitingList*
	* Envoie un signal à un résident/visiteur losqu'il arrive à son étage

3. Processus Résident
	* S'enregistre dans la *DwellerList*
	* Se connecte à la file de message *Visiteur-Résident*
	* Donne ou non son accord pour l'accès d'un visiteur
	
4. Processus Visiteur
	* Consulte la *DwellerList* pour savoir à qui demander l'accès
	* Envoie un message dans la file de message *Visiteur-Résident*
	* Si l'accord est donné, l'immeuble le redirige vers un des ascenseurs
	* S'enregistre dans la *WaitingList* de l'ascenseur en question
	* Si reception d'un signal *SIGUSR1*, entre dans l'ascenseur
	* Si reception d'un signal *SIGUSR2*, quitte l'ascenseur

## Avancement

### Dernièrement...

* *WaitingList* instanciée par le processus Ascenseur 
* Ajout des signaux SIGINT pour tous les processus

### A faire...

* Une fois l'autorisation donnée, le visiteur contacte l'immeuble pour savoir quel ascenseur utiliser
* Le visiteur doit s'enregistrer dans la *WaitingList* du bon ascenseur
* L'ascenseur doit parcourir les étages et ramasser les visiteurs au passage

## Liens utiles

A propos des Shared Memories:
http://icps.u-strasbg.fr/~bastoul/teaching/systeme/docs/TD4_memoire_partagee.pdf

tableaux en shared memory:
https://forum.hardware.fr/hfr/Programmation/C/stockage-memoire-partagee-sujet_115033_1.htm
