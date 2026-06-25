# Aethelgard

*Survivez. Créez. Explorez. Dans un monde qui n'est pas le vôtre.*

---

## Concept

**Aethelgard** est un jeu de survie multijoueur en monde ouvert avec une forte composante de crafting, se déroulant dans un univers de type **isekai** — vous êtes invoqué dans un monde fantastique hostile où tout est à reconstruire.

Dans cet exil forcé, vous n'êtes pas un héros légendaire. Vous êtes un survivant parmi d'autres, plongé dans un écosystème vivant et impitoyable où chaque créature, chaque biome et chaque choix compte.

## Univers

Aethelgard est un monde voxel où cohabitent dragons, félins, créatures marines, insectoïdes et bien d'autres formes de vie. Les biomes varient des plaines tempérées aux déserts ardents, des océans profonds aux montagnes glacées. La météo dynamique, le cycle jour/nuit et les dangers environnementaux (lave, gaz toxiques, tempêtes) rendent chaque expédition unique.

## Gameplay

- **Survie** — Gérez vos besoins vitaux (faim, soif, oxygène, température...) via un système data-driven où chaque race peut avoir ses propres besoins.
- **Crafting & Construction** — Récoltez des ressources, débloquez des recettes et bâtissez votre refuge dans un monde entièrement destructible et modifiable (voxel).
- **Combat** — Système de combat basé sur les parties du corps, 12 éléments de dégâts, capacités configurables et phases de boss.
- **Progression & Classes (isekai)** — Débloquez des classes, progressez dans des arbres de compétences et personnalisez votre build via un système de jobs inspiré des univers isekai.
- **Multijoueur** — Coopérez ou affrontez d'autres joueurs dans un monde persistant.

## Architecture technique

Le projet repose sur **Unreal Engine 5.x** avec une architecture orientée composants.

### Principes clés

- **Data-driven** — Toute donnée d'équilibrage (stats, besoins, loot, capacités) vit dans des Data Assets héritables. Modifier un parent impacte tous les enfants.
- **Comportement uniquement en C++** — Une classe C++ n'existe que si elle ajoute du comportement (méthode ou interface). Les espèces sont des instances de données (`URaceData`), pas des classes.
- **Système de corps modulaire** — Chaque entité vivante possède un `BodyPlan` définissant ses parties (tête, torse, ailes, pattes...) avec leurs propres points de vie et résistances.
- **Capacités data-driven** — Les sorts et compétences sont définis comme des Data Assets avec catégorie, élément, méthode de délivrance (mêlée, projectile, AoE, beam) et effets polymorphes.

### Stack

| Domaine         | Technologie                  |
|-----------------|------------------------------|
| Moteur          | Unreal Engine 5.x / C++      |
| Monde           | Voxel (chunks)               |
| Persistance     | SaveGame natif UE            |
| IA              | Behavior Trees + Perception  |
| Réseau          | Replication UE               |

Voir `UML.mermaid` pour le diagramme de classes complet.

## Licence

Projet privé — tous droits réservés.
