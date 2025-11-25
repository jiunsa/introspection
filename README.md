# Introspection - Web Directory Scanner

![C](https://img.shields.io/badge/C-00599C?style=for-the-badge&logo=c&logoColor=white)
![libcurl](https://img.shields.io/badge/libcurl-073551?style=for-the-badge&logo=curl&logoColor=white)
![pthread](https://img.shields.io/badge/pthread-FCC624?style=for-the-badge&logo=linux&logoColor=black)
![License](https://img.shields.io/github/license/jiunsa/introspection?style=for-the-badge)

## Description

Introspection est un scanner de répertoires web écrit en C qui teste l'existence de chemins sur un serveur web en utilisant une wordlist.

## Fonctionnalités

- Scanner multi-threadé (50 threads par défaut)
- Support HTTP/HTTPS
- Rotation aléatoire des User-Agent
- Détection des codes de statut HTTP (200, 301, 403)
- Mise en surbrillance des mots-clés sensibles (admin, login, etc.)
- Barre de progression en temps réel
- Résolution DNS et affichage de l'IP

## Prérequis

- GCC (compilateur C)
- libcurl (bibliothèque de transfert URL)
- pthread (bibliothèque de threads POSIX)

### Installation des dépendances

**Debian/Ubuntu:**
```bash
sudo apt-get install build-essential libcurl4-openssl-dev
```

**Fedora/RHEL/CentOS:**
```bash
sudo dnf install gcc make libcurl-devel
```

**Arch Linux:**
```bash
sudo pacman -S gcc make curl
```

**macOS:**
```bash
brew install curl
```

## Compilation

### Avec Make
```bash
make
```

### Compilation manuelle
```bash
gcc -Wall -Wextra -pthread -O2 -o introspection introspection.c -lcurl -lpthread
```

## Installation (optionnel)

```bash
sudo make install
```

## Utilisation

```bash
./introspection <URL_cible>
```

### Exemple
```bash
./introspection http://example.com
```

## Fichiers requis

- `wordlist.txt` : Fichier contenant la liste des chemins à tester (un par ligne)

## Codes de statut HTTP

- **200 OK** (vert) : Ressource trouvée et accessible
- **301 REDIRECT** (bleu) : Redirection détectée
- **403 RESTRICTED** (rouge) : Accès interdit

## Mots-clés détectés

Les chemins contenant ces mots-clés sont mis en surbrillance :
- admin
- login
- private
- administration
- secure
- wp, wp-admin, wp-login
- prive
- robot
- .htaccess, .htpassword, passwd
- .ht

## Performance

- Scan multi-threadé avec 50 threads parallèles
- Timeout de 5 secondes par requête
- Rotation automatique des User-Agent pour éviter la détection

## Nettoyage

```bash
make clean
```

## Désinstallation

```bash
sudo make uninstall
```

## Avertissement

Cet outil est destiné uniquement à des fins éducatives et de test de sécurité sur des systèmes dont vous avez l'autorisation. L'utilisation de cet outil sur des systèmes sans autorisation explicite est illégale.

## Différences avec la version Python

- Performances améliorées grâce à la compilation native
- Utilisation de libcurl au lieu de requests
- Gestion des threads POSIX au lieu de ThreadPoolExecutor
- Pas de dépendances externes à installer (hormis libcurl système)

## Auteur

OdG

## Licence

Voir le fichier LICENSE
