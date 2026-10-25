# Origem das continuações e identidade dos scripts

Data: 2026-10-25. Pesquisa estática no ELF ARM64 já identificado; VAs sem a base Ghidra0x100000. Nenhuma execução do jogo.

## Identificação do script adiado

AppData.GetDelayId34384, VA0x110caa0, foi exportado para `build/research/delay-origin/M34384.asm`. Percorre Data.eventData em ordem crescente, ignora registros null e compara **o ponteiro** de execsScript_ (+0x40) com o argumento. Retorna o primeiro índice correspondente. Não compara linhas, valores ou ID interno do registro.

Se não encontra correspondência, acessa o array AppData+0x1e8 e percorre sua quantidade, mas o corpo observado não compara seus elementos nem produz outro ID. Após diagnóstico condicional, retorna-1. Um array null nesse caminho falha; não converter silenciosamente essa situação em uma busca bem-sucedida. O alvo exato do diagnóstico ainda não foi interpretado.

Consequência para o port: clonar uma matriz e procurar outra de conteúdo igual não reproduz o contrato de identificação. O loader e o executor precisam conservar a identidade do script associado ao registro. Retornar-1 também não significa cancelar o agendamento: ExecuteScript34377 continua construindo e enfileirando DelayEvent, conforme77; a resolução posterior de-1 tem o comportamento inválido descrito em78.

## Produtor e campo do formulário

Metadata do tipo4296/field22705 e tabela de offsets confirmam SubForm.delayEvent_ em+0x1d0. O consumidor SubForm.Finish permanece confirmado, mas a atribuição não foi localizada.

Busca de BL/B imediato nos segmentos executáveis encontrou um caller direto do construtor DelayEvent35499 (VA0x11c9508): AppData.ExecuteScript34377, site0x110ca5c. Nesse caminho, o objeto é adicionado à coleção AppData+0x200; não é associado diretamente ao SubForm.

Para procurar escritores de+0x1d0, foram examinados STR64 com offset unsigned imediato, STP64 com offset imediato cobrindo+0x1c8/+0x1d0 e ADD64 imediato do endereço+0x1d0. Entre funções reconhecidas pelo mapa como Assembly-CSharp, os três STR candidatos pertencem a BootForm.Load, QuestForm..ctor e SelectForm.Init; são outros tipos, não prova de escrita no SubForm. Os dois outros padrões não produziram candidatos nesse filtro. Artefatos locais: `subform-store-candidates.json` e `subform-pair-address-candidates.json`.

Os três construtores SubForm36130–36132 também foram exportados e não apresentam atribuição direta a delayEvent_. Isso **não prova ausência de escritores**: a busca não cobre toda computação de endereço, escrita indireta, reflexão, funções sem identificação ou todas as variantes de instrução. Não declarar esse campo morto ou implementar uma associação automática ao fechar falas sem evidência.

## Caminho presente na tabela extraída

Auditoria das matrizes do Event.txt inglês já interpretado em76, registrada em `build/research/event-delay-usage.json`:

| Espera informada | Quantidade de instruções opcode10 |
| --- | --- |
| 1 | 1 |
| 3 | 1 |
| 5 | 21 |
| 30 | 10 |
| 50 | 4 |

Total:37. Nenhuma linha contém terceiro argumento de hora; todos os IDs dos registros correspondentes estão no intervalo aceito pelo resolver. Esses dados descrevem a tabela extraída, não comprovam que todas as instruções sejam alcançadas em execução ou que não existam outros produtores.

O construtor recebe hour=-1 nesse caminho, e a fila de SelectForm decrementa wait apenas quando seu trecho é alcançado. Portanto a próxima implementação pode abordar o contrato demonstrado de execução da matriz e continuação por contagem, preservando as guardas externas. Não converter esses números para segundos nem tratar todo Update como passagem válida.

## Estado e próximo passo

O ciclo anterior foi progresso: organização e Git validados. Este ciclo resolveu a identidade de GetDelayId e delimitou o caminho de espera efetivamente presente nas tabelas, permitindo priorizar integração sem bloquear tudo na rota de formulário ainda não explicada.

Próximo: portar executor de matriz/continuação por contagem com identidade explícita, ordem de efeitos e testes de interrupção, retomada e agendamento durante a passagem. Manter os efeitos dos opcodes e guardas de plataforma separados até estarem comprovados. Sem alterações C++ nesta rodada; não houve repetição de testes. Os builds Windows mais recentes permanecem10/10 em cada configuração, conforme80.
