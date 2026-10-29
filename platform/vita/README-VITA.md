> Historical implementation notes. For the current candidate, build and validation
> status see the root README and docs/HARDWARE-TEST.md. No historical statement
> below certifies the current reconstructed package on hardware.

# Minish Cap Vita 0.06

Portage natif Vita issu de EstebanPdN/zelda-tmc-3ds,
commit `57fe1a998761d5c1021904cee3cca1067989ba5a`, descendant de Project Picori
et zeldaret/tmc. Le code C du jeu tourne sur le processeur Vita.
Les noms hérités `Platform3DS` désignent l'interface de plateforme.

## Installation et commandes

Installer `minishcap-vita-0.06.vpk` avec VitaShell par-dessus la version précédente.
La ROM européenne reste à `ux0:data/minishcap/baserom_eu.gba`
(SHA-1 `cff199b36ff173fb6faf152653d1bccf87c26fb7`, 16 MiB).
Le VPK ne contient aucune ROM. Le français se choisit dans le jeu.

Croix = A GBA, Rond = B GBA, directions/stick gauche = déplacement,
L/R, Start et Select = boutons GBA correspondants.
**Triangle ouvre les paramètres du portage**, avec jeu et mixage audio en pause.
Haut/bas sélectionne, Croix ouvre une rubrique ou valide, gauche/droite modifie.
Rond revient, Triangle/Start ferme, Carré affiche l’aide.
Les touches sont neutralisées jusqu'au relâchement, sticks dans leur zone morte.

Options prises en charge : FPS, stick gauche, volume 0–100 %, taille d'image
3x / Max / 2x, filtre net/lissé et correction des couleurs GBA.
Max conserve les proportions (816 × 544), 3x donne 720 × 480 centré,
2x donne 480 × 320. Le menu reste en 3x pour sa lisibilité.
Les réglages s'appliquent à la fermeture et sont conservés dans
`ux0:data/minishcap/vita-menu.ini` avec une copie `.bak`.
En cas d'erreur d'écriture, le menu reste ouvert ; Start permet de reprendre
avec les réglages en mémoire, sans prétendre les avoir enregistrés.

Le profil utilise au premier démarrage les valeurs existantes volume/couleurs/FPS
de `minishcap.ini`, puis `vita-menu.ini` devient prioritaire pour ces options.
Les autres réglages hérités restent dans `minishcap.ini`.
Sauvegarde du jeu : `tmc_eu.sav` (EEPROM 8 KiB), jamais modifiée par le menu.
Journaux : `vita.log`, `stdout.log`, `diagnostic.txt` dans le même dossier.

## Architecture commune

`port-menu/` est un module autonome C11, API 1, version 0.1.0 : options par
identifiant, pagination, actions et confirmations, callbacks, thème et persistance.
`vita_menu.c` déclare seulement les options de Minish Cap et leur application.
`vita_platform.c` assure la boucle modale, l'affichage et les entrées Vita/SDL.
`vita_audio.c` synchronise la pause avec le worker, transmet du silence sans
faire avancer le mixeur, puis reprend sans le réinitialiser.
Voir `port-menu/README.md` pour un nouveau jeu. Donjon Mystère n'est pas modifié.
Les fonctions spécifiques (turbo, restauration de sauvegarde, etc.) nécessitent
un adaptateur du jeu ; elles ne sont pas proposées sans implémentation.

## Ajouts 0.06

Copies classées par génération, de la plus récente à la plus ancienne, avec
date complète et secondes. Actions affichées sur une ligne entière ; aide et
confirmation identifient la copie sélectionnée. « Avant restauration » rétablit
la copie `.before-restore` compatible 0.04/0.05, après avoir conservé le fichier
actuel dans la rotation. Ferme le jeu après réussite. Les trois parties sont
concernées ; progression non sauvegardée perdue. Le fichier avant restauration
reste intact. La date affichée pour ce fichier est celle de sa mise de côté.

Module commun 0.3, modèle et classement des copies également utilisés dans
Pokémon 0.46. Son rendu Vita natif, ses traductions et son format de stockage
restent gérés par l'adaptateur Pokémon. Sources communes identiques vérifiées
par `sync_port_menu.py`. Tests ASan/UBSan et restauration via menu hôte réussis.
Nouveaux VPK encore à essayer sur Vita. L'utilisateur a confirmé que LiveArea,
Settings et son après veille fonctionnent sur la 0.05.

## Ajouts 0.05 : LiveArea

Icône, forêt en fond, vignette avec le logo original et bouton Settings.
Assets 8-bit indexed PNG, dimensions contrôlées avant packaging. Sources
visuelles dans `assets-source`, script reproductible `build_livearea.py`
(Pillow et police macOS indiquée dans le script). Le fond est rendu par le jeu
sans les couches du logo, avec l'option hôte `TMC_TITLE_BACKGROUND=1`.

Même lancement que Pokémon : `psla:-settings`, lu via `sceAppMgrGetAppParam`,
ouvre le menu partagé avant `AgbMain`. Fermer le menu continue le démarrage.
Les actions de sauvegarde restent indisponibles tant que le moteur n'a pas
chargé la partie. Rafraîchissement LiveArea depuis `app0:` à la première
exécution de la nouvelle révision ; échec non bloquant, réessayé au lancement
suivant. Le bouton apparaît après installation ou après ce rafraîchissement.

Pour tester Settings, fermer complètement le jeu puis utiliser le bouton.
Pendant une partie, Triangle reste l'accès aux réglages. Le lancement à froid
est validé sur hôte (image/audio/états identiques après fermeture du menu) ;
le dispatch de la bulle déjà ouverte et le placement réel demandent un essai
Vita. Les modifications du moteur 0.04 ne changent pas dans cette version.

## Ajouts 0.04

Six rubriques : Affichage, Commandes, Audio, Général, Performances, Sauvegardes.
Réassignation de Croix/Rond/Carré/L/R/Start/Select ; Triangle et les commandes
internes du menu restent fixes. Zone morte du stick réglable de 10 à 60 %.
Réinitialisation des réglages avec confirmation, sans supprimer les parties.
Démarrage rapide facultatif vers le choix de fichier au prochain lancement.

Trois copies automatiques distinctes de la sauvegarde du jeu, après écriture
complète : `tmc-backup.0.pmb` à `.2.pmb`. Elles comprennent les trois emplacements
de partie. « Copier maintenant » copie la dernière sauvegarde sur disque ; ce
n'est pas une sauvegarde instantanée. La restauration demande confirmation,
préserve le fichier actuel dans `tmc_eu.sav.before-restore`, puis ferme le jeu.
La progression non sauvegardée en mémoire est perdue. Une copie dont la taille
ou le CRC est incorrect n'est pas restaurable. Récupération des remplacements
interrompus au démarrage, sans écraser un cas ambigu.

Notifications de veille/reprise : pause cumulée avec celle du menu, remise à
zéro de l'horloge, relâchement des commandes et réouverture de la sortie audio.
Mesures sur environ 60 secondes : FPS, rendu logiciel, présentation, pire durée
et images >20 ms. Export par le menu vers `ux0:data/minishcap/performance.csv`.
La présentation inclut attente et copie ; ce n'est pas un compteur GPU isolé.
Ces mesures servent au diagnostic ; aucune accélération n'est annoncée ici.

Tests 0.04 : ASan/UBSan (menu, stockage et helpers), pauses audio imbriquées,
comparaison hôte sur 14 510 frames avec pause/menu/veille simulée (mêmes états,
audio, sauvegarde et image), restauration réelle via le menu, aide/defaults,
démarrage normal/rapide et export CSV. Parcours ARM des escaliers : 18 000 frames.
La veille physique, le retour audio Vita et les performances restent à vérifier.

## Compilation

Use the current [ROM reconstruction guide](../../docs/REBUILD.md). Direct Vita
packaging requires both `--generated-source-dir` and `--assets-dir`, pointing
to private generated inputs outside Git. The historical commands without those
arguments are not supported. The host CMake build also needs
`-DTMC_GENERATED_DIR=/path/to/generated` and SDL2.

Version hôte : SDL2. Lancer depuis un répertoire de test isolé (sauvegarde locale).
T ouvre le menu, X valide, Z revient, C affiche l’aide. Paramètres de test : `TMC_FRAMES`,
`TMC_UNPACED`, `TMC_INPUT_SCRIPT`, `TMC_DUMP_AT`, `TMC_TRACE`, `TMC_AUTOLOAD=0`.
`TMC_AUDIO_CAPTURE=audio.raw` produit un mixage synchrone reproductible.
`TMC_MENU_AT` ouvre le menu à une frontière de frame ; `TMC_MENU_SCRIPT`
utilise des intervalles début/fin exclusifs et des masques PM_* hexadécimaux,
sur une horloge modale séparée. `TMC_SUSPEND_AT` simule une pause système. `TMC_MENU_DUMP` enregistre son image PPM.
Ces commandes de scénario modal n'existent que dans la compilation hôte.

## Validation et limites

0.02 corrige sept tests de suppression d'entités : comparaison exacte avec -1,
pour préserver les pointeurs Vita ayant le bit fort à 1. Parcours ARM validé
jusqu'au village, la Peste Mojo, la cérémonie du château, l'épée et deux Octoroks.
Le rendu de ce test était échantillonné et les services système simulés.

0.03 : tests du module avec ASan/UBSan ; réglages, erreurs et récupération ;
trois cycles de pause du mixeur SDL ; chargement des six options au redémarrage.
Deux exécutions hôtes, avec et sans ouverture du menu, donnent les mêmes états,
la même image finale, le même flux audio synchrone et la même sauvegarde de test.
Le jeu avance après la fermeture. Le binaire ARM 0.03 passe aussi l'aller-retour
chambre/entrée/chambre (18 000 frontières de frame, Unicorn).

0.03 : l’utilisateur confirme que tout fonctionne sur sa Vita.
0.04 : tests locaux validés ; nouvelle version encore à vérifier sur la console. Les tests hôtes et ARM
ne valident pas la sortie audio Vita, ses performances réelles, la veille/reprise
ou l'ensemble de l'aventure. Le PPU logiciel reste monothread. CPU : 444 MHz,
audio : 48 kHz stéréo, cadence visée : environ 59,73 Hz.

Licence du portage : GPL-3.0-or-later selon les mentions du projet parent.
Le nouveau module autonome est sous MIT ; sa police conserve sa licence BSD.
Les autres composants conservent leurs licences (voir PROVENANCE.txt et licenses).
