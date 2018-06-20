
### Comment exécuter le code ?

Il n'y a malheureusement pas de "Launcher" pour le moment.
Il faut donc lancer les processus 1 à 1 et si possible dans l'ordre suivant :
	* Immeuble (./Immeuble)
	* Ascenseurs (./Ascenseur [id])
	* Résidents (./Resident [floor] [door])
	* Visiteurs (./Visiteur [floor] [door])

### Comment réagit le programme ?

* Un visiteur est créé et questionne le résident chez qui il veut se rendre (Pour accepter une demande, entrez 'o')
* Si le résident autorise l'acccès, l'immeuble attribue le visiteur à un ascenseur
* L'ascenseur emmène le visiteur jusqu'au bon étage
* Le visiteur quitte l'ascenseur, se rend chez le résident puis décide de repartir plus tard
* Il reprend l'ascenseur pour sortir puis meurt.
