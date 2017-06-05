Rapport Projet Système
=================================

Thomas HAUTIER - Joan FLECHEUX

## Contenu

Notre projet implémente les conduits anonymes et nommés.
Cependant les conduits nommés ne fonctionnent pas entre processus. (malgré l'implémentation du tampon et de mutex en partagé).

Le projet est accompagné de plusieurs exemple:

- **exemple1.c** : Exemple basique avec un write et un read.
- **exemple1_2.c** : Identique au premier mais avec un conduit nommé.
- **exemple2.c** : Exemple avec des écritures/lectures concurrentes.
-**exemple3_client.c/exemple3_serveur.c**: Exemple montrant la communication entre processus.

L'implémentation a été testé avec succès sur un Rasberry Pi et sur un BeagleBone Black.


## Compilation

# TODO