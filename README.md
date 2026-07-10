## O Problema dos Fumantes (The Cigarette Smokers Problem)

Este repositório contém a implementação da solução para o clássico **Problema dos Fumantes**, como parte das atividades práticas da disciplina de Sistemas Operacionais (MC504).

### Integrantes
*   **Fernando Kenzo Imami Fugihara** — RA: 205067
*   **Fabio Luiz Ferreira da Silva Junior** — RA: 247079

---

## Descrição do Problema

O problema consiste em três fumantes e um agente. Cada fumante possui uma quantidade infinita de um ingrediente específico:
1.  **Fumante 0:** Possui **Tabaco** (necessita de Papel e Fósforo)
2.  **Fumante 1:** Possui **Papel** (necessita de Tabaco e Fósforo)
3.  **Fumante 2:** Possui **Fósforo** (necessita de Tabaco e Papel)

O **Agente** coloca aleatoriamente dois ingredientes diferentes na mesa. O fumante que possui o terceiro ingrediente restante deve recolher os itens da mesa, montar seu cigarro, fumar por um período de tempo e, ao terminar, liberar a mesa para que o agente possa distribuir novos ingredientes.

### O Papel dos Combinadores (Pushers)
Uma implementação direta utilizando apenas semáforos simples entre o agente e os fumantes pode levar a *deadlocks* ou à escolha incorreta de qual thread acordar. Para mitigar isso, foram implementadas as threads **Combinadoras**. Elas monitoram quais ingredientes foram disponibilizados pelo agente e realizam a lógica de ativação de forma segura para acordar exatamente o fumante correto.

---

## Como Compilar e Executar

```bash
gcc main.c
./a.out
```