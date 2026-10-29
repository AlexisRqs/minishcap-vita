# Port Menu 0.3 — module commun aux portages

C11, API `PM_API_VERSION=2`, sans dépendance à VitaSDK, SDL ou aux données des jeux. Libc POSIX (`fsync`, `access`, `rename`) nécessaire pour les fichiers. Sources MIT ; police BSD PSPSDK conservée avec sa licence. Le profil Minish Cap est dans `../vita_menu.c`. Pokémon 0.46 et Minish Cap 0.06 utilisent ce module via leurs adaptateurs respectifs.

## Intégration

Compiler les sources utiles, ou `add_subdirectory(port-menu)` puis `target_link_libraries(mon_jeu PRIVATE port_menu)`. `port_menu.c` et `port_model.c` suffisent au menu. `port_backup.c` et `port_runtime.c` sont facultatifs et indépendants.

```c
static PM_Option options[] = {
 {.id="volume", .label="Volume (%)", .type=PM_RANGE,
  .value=100, .minimum=0, .maximum=100, .step=10,
  .category="Audio", .help="Volume du jeu."},
 {.id="show_fps", .label="Afficher les FPS", .type=PM_BOOL,
  .value=0, .minimum=0, .maximum=1, .step=1,
  .category="Affichage", .help="Nombre d'images presentees par seconde."}
};
static PM_Menu menu;
if (!PM_Init(&menu, "Mon jeu", "menu.ini", options, 2, contexte)) {
    /* Profil invalide : signaler une erreur d'integration. */
}
PM_Load(&menu);
```

Conserver tableaux, chaînes et contexte pendant la vie du menu. Identifiants ASCII uniques et stables ; jusqu'à 32 options. `PM_BOOL`, `PM_RANGE`, `PM_CHOICE`, `PM_ACTION` ; callbacks `enabled`, `action` et confirmation facultative. `PM_Reset` rétablit les valeurs présentes lors de `PM_Init`, sans toucher aux sauvegardes du jeu. Les actions doivent gérer elles-mêmes leurs effets.

Toutes les options ont une catégorie non vide, ou aucune n'en a. Un profil mixte est refusé. Sans catégories, navigation plate compatible avec les anciens profils après recompilation. L'ordre de première apparition détermine l'ordre des rubriques. Pagination de sept lignes. `help` et `confirmation` affichent jusqu'à huit lignes de 28 caractères ; écrire des textes ASCII courts. Police ASCII 8x8, libellés de 16 caractères, valeurs de 8. Rendu dans un tampon RGBA8 240x160, stride >=240 ; couleurs configurables par `menu.theme` (mots ABGR).

À une frontière sûre du moteur : suspendre la logique et le mixeur, appeler `PM_Open(menu, touches_deja_tenues)`, puis `PM_Update`/`PM_Draw` à chaque itération modale. Adapter les boutons physiques aux bits `PM_*`, indépendamment des commandes personnalisées du jeu. X valide, O revient, Triangle/Start ferme, Carré ouvre l'aide. Les confirmations et l'aide consomment d'abord la touche de retour. Après fermeture, appliquer les réglages et attendre le relâchement des commandes avant de reprendre les entrées du jeu.

Les paramètres `port_menu=1` restent compatibles avec 0.1 : identifiants inconnus et valeurs hors bornes ignorés. Écriture `.tmp`, synchronisation, rotation `.bak`, remplacement ; récupération du `.bak` si le principal est absent ou d'en-tête invalide. En cas d'échec d'écriture, le menu reste ouvert ; Start autorise une sortie explicite sans enregistrement.

## Sauvegardes de secours

`PB_Store` fournit le chemin de sauvegarde, un préfixe **propre au jeu et à sa région**, sa taille fixe et un validateur facultatif. Un validateur du format réel est recommandé. Appeler `PB_Recover` au démarrage, avant que le moteur charge sa sauvegarde, mais après initialisation des données nécessaires au validateur. En cas d'échec, préserver les fichiers et interrompre le chargement.

Appeler `PB_Capture` uniquement après une sauvegarde complète, synchronisée, hors transaction. Trois slots `${prefix}.0.pmb` à `.2.pmb`, rotation par génération, déduplication de la copie la plus récente. En-tête 32 octets : magie PMBACK1, taille, CRC32 des données, génération, timestamp et CRC32 du chemin source. Ce contrôle détecte des corruptions accidentelles, ce n'est pas une authentification. Les métadonnées de date/génération ne sont pas authentifiées. `PB_InfoAt` vérifie taille et intégrité ; le validateur métier est appelé à la capture et à la restauration.

`PB_Restore` conserve le fichier actuel dans `${path}.before-restore`, valide la copie temporaire puis la remplace avec un fichier de retour arrière. Un fichier actuel est requis. Le jeu doit ensuite **fermer ou recharger entièrement son état**, pour empêcher une réécriture depuis une ancienne progression en RAM. Les backups couvrent le fichier entier, pas un emplacement individuel dans le jeu. Minish Cap ferme après confirmation ; ses trois emplacements sont restaurés ensemble. Pas de sauvegarde instantanée de la mémoire.

Les remplacements utilisent `.tmp`, `.old` et `.restore.rollback`. `PB_Recover` récupère les rotations interrompues ; si le principal et un rollback sont présents mais que le principal échoue à la validation, il laisse les deux intacts et signale un échec. Aucun accès concurrent : l'adaptateur doit sérialiser lecture, capture et restauration. Chemins limités à 280 caractères, images à 16 Mio.

## Commandes, pause et mesures

`PR_MapButtons` traduit un masque de boutons physiques via une table d'indices vers des masques choisis par le portage ; plusieurs boutons peuvent partager une action. `PR_Stick` utilise les bits directionnels GBA (16 droite, 32 gauche, 64 haut, 128 bas) : les traduire dans l'adaptateur pour une autre convention. Axe Y positif vers le bas, zone morte par axe.

`PR_SetPause` compose les raisons MENU et SYSTEM. Le jeu ne reprend que lorsque toutes sont levées. `PR_ReleaseInputs` bloque jusqu'à un échantillon neutre, puis autorise le suivant. Le module ne suspend pas le moteur ou le son : ces opérations appartiennent à l'adaptateur. Minish Cap enregistre un callback Vita d'alimentation, traite les notifications à une frontière sûre, réinitialise l'horloge de cadence et rouvre la sortie audio après reprise.

`PR_ProfileFrame` reçoit une horloge monotone en microsecondes, les coûts du rendu et de présentation, et un identifiant de scène. Agrégation par fenêtres d'environ une seconde, historique circulaire de 60 fenêtres, FPS, moyennes, pire durée et images >20 ms. `PR_ProfileResetClock` exclut les pauses sans effacer l'historique. `PR_ProfileExport` écrit un CSV à la demande. Le temps de présentation inclut copie et attente ; ce n'est pas une mesure GPU isolée. Les résultats hôtes ne préjugent pas des performances Vita.

## Tests

```sh
cc -std=c11 -D_POSIX_C_SOURCE=200809L -I. port_menu.c port_model.c tests/menu_test.c -o menu-test
cc -std=c11 -D_POSIX_C_SOURCE=200809L -I. port_backup.c port_runtime.c port_model.c tests/runtime_test.c -o runtime-test
mkdir -p test-output/storage
./menu-test test-output
./runtime-test test-output/storage
```

Utiliser un dossier neuf pour chaque exécution du test de stockage. Compatible ASan/UBSan. Tests des catégories, aide, pagination, confirmations, defaults, persistance, erreurs d'écriture, rotation, corruption, restauration, récupération, mappings, zone morte, pauses imbriquées et historique de mesures. Le test du mixeur et l'intégration Minish Cap sont dans `../tests`.

Origine de la police : `sample/debugScreenFont.c` du portage Donjon Mystère, issu de PSPSDK ; auteurs et licence conservés dans `font8x8.inc` et `FONT-LICENSE.txt`. Licence source : https://github.com/pspdev/pspsdk/blob/master/LICENSE.

## Ajouts 0.3 et intégration Pokémon

`port_model.c/.h` sépare les règles d'options et de navigation de leur rendu.
Minish Cap utilise `PM_AdjustOption` via le menu 240x160 ; Pokémon utilise la
même fonction, ainsi que `PM_Nav`, avec son rendu Vita natif, ses onglets,
sa traduction et son tactile. Les choix masqués sont filtrés à la navigation
et au toucher ; le rappel d'onglet mémorise des identifiants, pas des rangs.
Les options et valeurs propres au jeu restent dans son adaptateur. API de
menu 2 inchangée, fonctions de modèle ajoutées ; pas de nouveau format de
réglages. Pokémon conserve son fichier binaire et ses migrations existantes.

`PM_SortNewest` trie les copies valides par génération décroissante, même si
l'heure de la console change. `PM_FormatDate` produit une date complète avec
secondes (locale de la console). Les deux jeux utilisent ces fonctions.
Leurs formats de sauvegarde et contrôles métier restent distincts ; aucune
conversion ni réécriture des anciennes copies n'est nécessaire.

Minish Cap expose `PB_List` (infos et slots physiques triés) : toujours
restaurer le slot retourné, jamais le rang de la ligne affichée. Les actions
du menu utilisent maintenant une ligne entière, pour rendre les dates lisibles.
`PB_PreviousInfo` valide la copie raw `.before-restore`, compatible avec 0.04/0.05.
`PB_RestorePrevious` conserve d'abord la sauvegarde actuelle dans la rotation,
restaure cette copie raw et laisse `.before-restore` intact. Si la copie de
sécurité échoue, la sauvegarde actuelle n'est pas remplacée. Fermer le jeu
après réussite, comme pour `PB_Restore`. Le timestamp de ce fichier indique
sa mise de côté, pas nécessairement la date de progression dans le jeu.

Le test `tests/model_test.c`, compilé avec `port_model.c`, couvre navigation,
options cachées, groupes vides, bornes et choix circulaires. Les adaptateurs
Pokémon ajoutent des tests différentiels contre 0.45, du rendu et du tactile.
Les tests de stockage couvrent aussi le retour avant restauration et la
récupération d'un remplacement interrompu de `.before-restore`.

## Dépôt autonome et versions figées

La source commune est https://github.com/AlexisRqs/port-menu (dépôt privé).
La première version extraite est `v0.3.0`. Le code C et la police sont identiques
à la livraison 0.3 ; l'extraction ajoute les tests CMake et l'intégration Git.
Chaque portage enregistre un commit précis via son sous-module. Les adaptateurs,
options et formats de sauvegarde spécifiques restent dans les jeux.

```sh
git clone --recurse-submodules URL_DU_PORTAGE
# Après un pull du portage :
git submodule update --init --recursive
# Mise à jour volontaire, depuis le portage :
git -C CHEMIN_DU_MODULE fetch origin --tags
git -C CHEMIN_DU_MODULE checkout --detach v0.3.0
# Tester/recompiler le portage, puis enregistrer le nouveau pointeur :
git add CHEMIN_DU_MODULE
git commit -m "Update port-menu to v0.3.0"
```

Ne pas utiliser `submodule update --remote` pour un build reproductible.
Ne pas modifier la copie embarquée : développer dans le dépôt autonome,
publier une version, puis mettre à jour chaque portage séparément.
L'accès GitHub au dépôt privé est requis pour initialiser les sous-modules.

```sh
cmake -S . -B build -DPORT_MENU_BUILD_TESTS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Les tests utilisent des dossiers temporaires distincts. Ils peuvent être relancés.
Les builds des jeux qui emploient `add_subdirectory` n'activent pas les tests
par défaut. Les distributions de sources doivent inclure les fichiers du
sous-module : `git archive` du dépôt parent seul ne les inclut pas.

`sync_port_menu.py` est conservé pour les anciennes copies sans Git. Il exclut
les métadonnées Git et refuse d'écraser un checkout Git avec `--write`.
