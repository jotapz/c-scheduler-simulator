#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

typedef enum
{
    PRONTO,
    EM_EXECUCAO,
    BLOQUEADO,
    CONCLUIDO
} ProcessState;

typedef struct Process
{
    int id;
    int tempo_chegada;
    int tempo_execucao_total;
    int tempo_restante;
    ProcessState state;

    int tempo_primeira_execucao;
    int tempo_conclusao;
    int ja_bloqueou;
    int precisa_io;

    struct Process *next;
} Process;

Process *process_list = NULL;
int next_process_id = 1;

void terminal_initialize(void);
void terminal_writestring(const char *str);

void create_process(void);
void list_processes(void);
void terminate_process(int id);
void execute_processes_fcfs(void);
void execute_processes_rr(int quantum);
void execute_processes_sjf(void);

int main(void)
{
    int opcao;
    terminal_initialize();

    while (1)
    {
        terminal_writestring("\n--- simulador de escalonamento ---\n");
        terminal_writestring("1. executar processos com FCFS\n");
        terminal_writestring("2. executar processos com Round Robin\n");
        terminal_writestring("3. executar processos com SJF\n");
        terminal_writestring("4. criar processo\n");
        terminal_writestring("5. listar processos\n");
        terminal_writestring("6. terminar processo\n");
        terminal_writestring("7. sair\n");
        terminal_writestring("escolha uma opcao: ");

        scanf("%d", &opcao);

        switch (opcao)
        {
        case 1:
            execute_processes_fcfs();
            break;
        case 2:
        {
            int quantum;
            terminal_writestring("digite o valor do quantum: ");
            scanf("%d", &quantum);
            if (quantum > 0)
            {
                execute_processes_rr(quantum);
            }
            else
            {
                terminal_writestring("quantum invalido!\n");
            }
        }
        break;
        case 3:
            execute_processes_sjf();
            break;
        case 4:
            create_process();
            break;
        case 5:
            list_processes();
            break;
        case 6:
            terminal_writestring("digite o id do processo a ser terminado: ");
            int id;
            if (scanf("%d", &id) == 1)
            {
                terminate_process(id);
            }
            break;
        case 7:
            terminal_writestring("sistema finalizado.\n");
            exit(0);
            break;
        default:
            terminal_writestring("opcao invalida!\n");
        }
        sleep(1);
    }
    return 0;
}

void create_process(void)
{
    Process *new_process = (Process *)malloc(sizeof(Process));
    if (!new_process)
    {
        terminal_writestring("erro: falha ao criar processo!\n");
        return;
    }

    new_process->id = next_process_id++;

    terminal_writestring("digite o tempo de chegada do processo: ");
    scanf("%d", &new_process->tempo_chegada);

    terminal_writestring("digite o tempo de execucao total do processo: ");
    scanf("%d", &new_process->tempo_execucao_total);

    terminal_writestring("precisara de entrada/saida (dados)? (1 - sim, 0 - nao): ");
    scanf("%d", &new_process->precisa_io);

    new_process->tempo_restante = new_process->tempo_execucao_total;
    new_process->state = PRONTO;

    new_process->tempo_primeira_execucao = -1;
    new_process->tempo_conclusao = -1;
    new_process->ja_bloqueou = 0;
    new_process->next = NULL;

    if (!process_list)
    {
        process_list = new_process;
    }
    else
    {
        Process *temp = process_list;
        while (temp->next)
        {
            temp = temp->next;
        }
        temp->next = new_process;
    }

    terminal_writestring("processo criado com sucesso!\n");
}

void list_processes(void)
{
    Process *current = process_list;
    if (!current)
    {
        terminal_writestring("nenhum processo ativo.\n");
        return;
    }

    terminal_writestring("lista de processos:\n");
    while (current)
    {
        printf("id: %d | estado: %s | chegada: %d | total: %d | restante: %d | e/s: %s\n",
               current->id,
               (current->state == PRONTO) ? "PRONTO" : (current->state == EM_EXECUCAO) ? "EM_EXECUCAO"
                                                   : (current->state == BLOQUEADO)     ? "BLOQUEADO"
                                                                                       : "CONCLUIDO",
               current->tempo_chegada,
               current->tempo_execucao_total,
               current->tempo_restante,
               (current->precisa_io == 1) ? "sim" : "nao");
        current = current->next;
    }
}

void terminate_process(int id)
{
    Process *current = process_list;
    Process *prev = NULL;

    while (current)
    {
        if (current->id == id)
        {
            if (prev)
            {
                prev->next = current->next;
            }
            else
            {
                process_list = current->next;
            }
            free(current);
            terminal_writestring("processo terminado com sucesso!\n");
            return;
        }
        prev = current;
        current = current->next;
    }
    terminal_writestring("erro: processo nao encontrado!\n");
}

void execute_processes_fcfs(void)
{
    if (!process_list)
    {
        terminal_writestring("nenhum processo para executar.\n");
        return;
    }

    int tem_pendente = 0;
    Process *check = process_list;
    while (check)
    {
        if (check->state != CONCLUIDO)
            tem_pendente = 1;
        check = check->next;
    }
    if (!tem_pendente)
    {
        terminal_writestring("todos os processos ja estao concluidos! crie novos ou reinicie o simulador.\n");
        return;
    }

    terminal_writestring("\n--- iniciando FCFS ---\n");
    Process *current = process_list;
    int tempo_atual = 0;

    float soma_turnaround = 0;
    float soma_resposta = 0;
    int qtd_processos = 0;

    while (current)
    {
        if (current->state == CONCLUIDO)
        {
            current = current->next;
            continue;
        }

        if (tempo_atual < current->tempo_chegada)
        {
            tempo_atual = current->tempo_chegada;
        }

        current->state = EM_EXECUCAO;
        if (current->tempo_primeira_execucao == -1)
        {
            current->tempo_primeira_execucao = tempo_atual;
        }

        printf("tempo %d: processo %d entrou em EXECUCAO\n", tempo_atual, current->id);

        while (current->tempo_restante > 0)
        {
            if (current->precisa_io == 1 && current->tempo_restante == current->tempo_execucao_total / 2 && current->ja_bloqueou == 0)
            {
                current->state = BLOQUEADO;
                printf("tempo %d: processo %d esta BLOQUEADO \n", tempo_atual, current->id);
                sleep(2);
                tempo_atual += 2;
                current->state = PRONTO;
                printf("tempo %d: processo %d recebeu os dados e voltou a fila de PRONTO\n", tempo_atual, current->id);
                current->state = EM_EXECUCAO;
                printf("tempo %d: processo %d retomou a EXECUCAO\n", tempo_atual, current->id);
                current->ja_bloqueou = 1;
            }

            sleep(1);
            tempo_atual++;
            current->tempo_restante--;
        }

        current->state = CONCLUIDO;
        current->tempo_conclusao = tempo_atual;
        printf("tempo %d: processo %d CONCLUIDO\n", tempo_atual, current->id);

        int turnaround = current->tempo_conclusao - current->tempo_chegada;
        int tempo_resposta = current->tempo_primeira_execucao - current->tempo_chegada;

        soma_turnaround += turnaround;
        soma_resposta += tempo_resposta;
        qtd_processos++;

        current = current->next;
    }

    printf("\n--- metricas finais (FCFS) ---\n");
    printf("turnaround medio: %.2f\n", soma_turnaround / qtd_processos);
    printf("tempo de resposta medio: %.2f\n", soma_resposta / qtd_processos);
    printf("processos concluidos: %d\n\n", qtd_processos);
}

void execute_processes_rr(int quantum)
{
    if (!process_list)
    {
        terminal_writestring("nenhum processo para executar.\n");
        return;
    }

    int tem_pendente = 0;
    Process *check = process_list;
    while (check)
    {
        if (check->state != CONCLUIDO)
            tem_pendente = 1;
        check = check->next;
    }
    if (!tem_pendente)
    {
        terminal_writestring("todos os processos ja estao concluidos! crie novos ou reinicie o simulador.\n");
        return;
    }

    terminal_writestring("\n--- iniciando Round Robin ---\n");
    int tempo_atual = 0;

    float soma_turnaround = 0;
    float soma_resposta = 0;
    int qtd_processos = 0;
    int processos_concluidos = 0;

    Process *temp = process_list;
    while (temp)
    {
        if (temp->state != CONCLUIDO)
        {
            qtd_processos++;
        }
        temp = temp->next;
    }

    while (processos_concluidos < qtd_processos)
    {
        Process *current = process_list;

        while (current)
        {
            if (current->state != CONCLUIDO && current->tempo_restante > 0 && current->tempo_chegada <= tempo_atual)
            {
                if (current->tempo_primeira_execucao == -1)
                {
                    current->tempo_primeira_execucao = tempo_atual;
                }

                current->state = EM_EXECUCAO;
                printf("tempo %d: processo %d entrou em EXECUCAO\n", tempo_atual, current->id);

                int tempo_rodando = (current->tempo_restante < quantum) ? current->tempo_restante : quantum;

                for (int i = 0; i < tempo_rodando; i++)
                {
                    if (current->precisa_io == 1 && current->tempo_restante == current->tempo_execucao_total / 2 && current->ja_bloqueou == 0)
                    {
                        current->state = BLOQUEADO;
                        printf("tempo %d: processo %d esta BLOQUEADO\n", tempo_atual, current->id);
                        sleep(2);
                        tempo_atual += 2;
                        current->state = PRONTO;
                        printf("tempo %d: processo %d recebeu os dados e voltou a fila de PRONTO\n", tempo_atual, current->id);
                        current->state = EM_EXECUCAO;
                        printf("tempo %d: processo %d retomou a EXECUCAO\n", tempo_atual, current->id);
                        current->ja_bloqueou = 1;
                    }

                    sleep(1);
                    tempo_atual++;
                    current->tempo_restante--;
                }

                if (current->tempo_restante == 0)
                {
                    current->state = CONCLUIDO;
                    current->tempo_conclusao = tempo_atual;
                    printf("tempo %d: processo %d CONCLUIDO\n", tempo_atual, current->id);
                    processos_concluidos++;

                    soma_turnaround += (current->tempo_conclusao - current->tempo_chegada);
                    soma_resposta += (current->tempo_primeira_execucao - current->tempo_chegada);
                }
                else
                {
                    current->state = PRONTO;
                    printf("tempo %d: processo %d pausado, voltou para fila PRONTO \n", tempo_atual, current->id);
                }
            }
            else if (current->state != CONCLUIDO && current->tempo_chegada > tempo_atual && processos_concluidos == 0)
            {
                tempo_atual++;
            }

            current = current->next;
        }
    }

    printf("\n--- metricas finais Round Robin ---\n");
    printf("turnaround medio: %.2f\n", soma_turnaround / qtd_processos);
    printf("tempo de resposta medio: %.2f\n", soma_resposta / qtd_processos);
    printf("processos concluidos: %d\n\n", qtd_processos);
}

void execute_processes_sjf(void)
{
    if (!process_list)
    {
        terminal_writestring("nenhum processo para executar.\n");
        return;
    }

    int tem_pendente = 0;
    Process *check = process_list;
    while (check)
    {
        if (check->state != CONCLUIDO)
            tem_pendente = 1;
        check = check->next;
    }
    if (!tem_pendente)
    {
        terminal_writestring("todos os processos ja estao concluidos! crie novos ou reinicie o simulador.\n");
        return;
    }

    terminal_writestring("\n--- iniciando SJF ---\n");
    int tempo_atual = 0;

    float soma_turnaround = 0;
    float soma_resposta = 0;
    int qtd_processos = 0;
    int processos_concluidos = 0;

    Process *temp = process_list;
    while (temp)
    {
        if (temp->state != CONCLUIDO)
        {
            qtd_processos++;
        }
        temp = temp->next;
    }

    while (processos_concluidos < qtd_processos)
    {
        Process *current = process_list;
        Process *shortest = NULL;
        int menor_tempo = 999999;

        while (current)
        {
            if (current->state != CONCLUIDO && current->tempo_chegada <= tempo_atual)
            {
                if (current->tempo_execucao_total < menor_tempo)
                {
                    menor_tempo = current->tempo_execucao_total;
                    shortest = current;
                }
            }
            current = current->next;
        }

        if (!shortest)
        {
            tempo_atual++;
            continue;
        }

        shortest->state = EM_EXECUCAO;
        if (shortest->tempo_primeira_execucao == -1)
        {
            shortest->tempo_primeira_execucao = tempo_atual;
        }

        printf("tempo %d: processo %d entrou em EXECUCAO (SJF - tempo total: %d)\n", tempo_atual, shortest->id, shortest->tempo_execucao_total);

        while (shortest->tempo_restante > 0)
        {
            if (shortest->precisa_io == 1 && shortest->tempo_restante == shortest->tempo_execucao_total / 2 && shortest->ja_bloqueou == 0)
            {
                shortest->state = BLOQUEADO;
                printf("tempo %d: processo %d esta BLOQUEADO\n", tempo_atual, shortest->id);
                sleep(2);
                tempo_atual += 2;
                shortest->state = PRONTO;
                printf("tempo %d: processo %d recebeu os dados e voltou a fila PRONTO\n", tempo_atual, shortest->id);
                shortest->state = EM_EXECUCAO;
                printf("tempo %d: processo %d retomou a EXECUCAO\n", tempo_atual, shortest->id);
                shortest->ja_bloqueou = 1;
            }

            sleep(1);
            tempo_atual++;
            shortest->tempo_restante--;
        }

        shortest->state = CONCLUIDO;
        shortest->tempo_conclusao = tempo_atual;
        printf("tempo %d: processo %d CONCLUIDO\n", tempo_atual, shortest->id);
        processos_concluidos++;

        soma_turnaround += (shortest->tempo_conclusao - shortest->tempo_chegada);
        soma_resposta += (shortest->tempo_primeira_execucao - shortest->tempo_chegada);
    }

    printf("\n--- metricas finais SJF ---\n");
    printf("turnaround medio: %.2f\n", soma_turnaround / qtd_processos);
    printf("tempo de resposta medio: %.2f\n", soma_resposta / qtd_processos);
    printf("processos concluidos: %d\n\n", qtd_processos);
}

void terminal_initialize(void)
{
    terminal_writestring("inicializando terminal...\n");
}

void terminal_writestring(const char *str)
{
    printf("%s", str);
}