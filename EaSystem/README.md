# Eä System

Aquest projecte té com a objectiu implementar un sistema de comunicació entre diferents processos anomenats **IluvatarSon**, connectats a través d’un servidor central anomenat **Arda**, permetent l’enviament d’informació entre processos en diferents servidors. El sistema utilitza diverses tècniques de programació a nivell de sistemes operatius, com ara **sockets**, **pipes**, **forks**, **signals** i **semàfors**.

<p align="center">
  <img width="350" height="250" src="https://encrypted-tbn2.gstatic.com/images?q=tbn:ANd9GcR4Nee7ek6T9l_vtwvliq0XMR0nnOxqZsJCUh1lKYcxmB5S7WVN">
</p>

## **Característiques principals**
- **Servidor central (Arda)**: Gestiona les connexions entre els processos IluvatarSon, monitora les seves desconnexions i manté un registre dels missatges enviats.
- **Processos client (IluvatarSon)**: Interactuen entre si a través d'Arda o directament, enviant missatges i fitxers.
- **Protocol de trames**: Comunicació basada en trames amb camps com tipus, longitud i dades.
- **Comunicació entre diferents servidors**: Els processos IluvatarSon estan desplegats en diferents màquines que representen diverses "terres".
- **Funcionalitat multithread**: Cada connexió d’un client a Arda genera un nou fil d’execució, optimitzant la gestió de peticions.

## **Tecnologies utilitzades**
- **Llenguatge de programació**: C.
- **Sistemes operatius**: Linux.
- **Comunicació entre processos**: Sockets, semàfors, pipes, senyals (SIGINT).
- **Mòduls**:
  - Arda (servidor central).
  - IluvatarSon (clients).

## **Instal·lació**
### Requisits
- **Compilador**: GCC o qualsevol compilador compatible amb C.
- **Sistema operatiu**: Distribucions Linux compatibles amb les crides al sistema utilitzades.
- **Makefile**: Per compilar tots els mòduls del projecte de manera eficient.

### Instruccions
1. Clona aquest repositori:
   ```bash
   git clone https://github.com/el-teu-usuari/easystem-practice.git
   ```
2. Compila el projecte utilitzant el fitxer **Makefile**:
   ```bash
   make
   ```
3. Executa el servidor Arda i els clients IluvatarSon des de diferents màquines o en la mateixa:
   ```bash
   ./arda
   ./iluvaterson
   ```

## **Com utilitzar**
1. **Inicia Arda**: Aquest servidor espera connexions dels IluvatarSon, genera un fil per a cada connexió i permet l’enviament de missatges i fitxers entre ells.
2. **Inicia IluvatarSon**: Els clients es connecten a Arda i poden interactuar entre si mitjançant trames enviades a través de sockets.
3. **Envia missatges o fitxers**: Utilitza les comandes personalitzades per enviar missatges o fitxers entre diferents processos.

## **Llicència**
Aquest projecte està sota la llicència BSD 3-Clause. Consulta el fitxer `LICENSE` per a més detalls.

---
