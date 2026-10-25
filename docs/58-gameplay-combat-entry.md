# Gameplay prioritário: entrada no combate e intervalos de ataque

Data:2026-10-25. Prioridade alterada explicitamente pelo usuário para gameplay; saves/localização ficam em segundo plano. Análise estática do mesmo ELF ARM64, sem executar APK. Endereços abaixo são VAs ELF; exports Ghidra somam0x100000.

## Onde investigar o combate

BattleSystem.Update37444 (`0x114236c`,308 bytes) obtém a coleção de seu subset e avança o enumerador até terminar, sem corpo de processamento entre MoveNext e a próxima iteração. Possui descarte e tratamento de exceção. Portanto o nome BattleSystem não justifica atribuir a esse Update a execução de ataques/dano. Não exclui efeitos dos helpers de coleção, mas o laço local não chama ataque.

HumanSystem.SearchAndAttack37698 (`0x1156074`,376 bytes) consulta StageSystem.get_isAttackActive, condições do atacante, procura alvo por EnemySystem.SearchNearestEnemy, aplica filtros ao alvo e, no ramo de sucesso, grava referência do alvo em componente do humano, zera três argumentos float em chamada de movimento e chama ChangeState com2. Retorna true nesse ramo, false nas guardas. Os nomes/semântica de todos os filtros e do estado2 ainda precisam ser resolvidos; não implementar aproximação por distância apenas.

EnemySystem.Damage37631 (`0x114fc44`,3.924 bytes) e Defeat37632 (`0x1150b98`,1.040 bytes) foram exportados e tiveram suas chamadas diretas inventariadas. Há evidência de dependência de parâmetros, atributos e aleatoriedade, não apenas subtração fixa de HP. Suas fórmulas completas ainda não foram recuperadas.

## Intervalo humano

HumanSystem.GetAttackSpeed37735 (`0x1158d94`,52 bytes) chama MemberSystem.GetTotalParamValue37817 com índice6. Seja P esse retorno: calcula a soma de32 bits P+100 e divisão inteira por2 com truncamento em direção a zero. As instruções ADD/CSEL/ASR preservam esse comportamento inclusive para somas negativas; não confundir com floor matemático para valores negativos. Overflow da soma segue32 bits.

GetAttackInterval37733 (`0x1155f08`,180 bytes) lê MyConfig.FPS, multiplica por100 em32 bits, divide pelo resultado acima via SDIV e faz tail call AppData.ClampMin34251 com mínimo2. Para valores sem overflow e divisor não zero:

```text
velocidade = trunc((P + 100) / 2)
intervalo = max(2, trunc(FPS * 100 / velocidade))
```

ClampMin (`0x1101848`,12 bytes) é comparação assinada seguida de seleção do maior valor. Não há guarda de divisor zero no corpo observado; qualquer port deve explicitar a semântica ARM64 de divisão e overflow, evitando comportamento indefinido C++. A fórmula acima não é especificação dos casos excepcionais de máquina.

MyConfig.FPS foi identificado por GOT0x27312d8, tipo4527, tabela nativa de offsets e campo24199/FPS no offset estático0x50. Seu valor efetivo e a frequência com que o intervalo é consumido não foram fixados nesta investigação. Não converter intervalo para segundos supondo50 ou60 FPS.

GetTotalParamValue37817 (`0x1156424`) soma, nesta ordem, retorno de MemberSystem.GetParamValue37819, valor da arma, escudo, chapéu e roupa, todos para o mesmo índice solicitado. Getters e métodos de dados foram associados pelo mapa IL2CPP; retornos são somados em32 bits. O significado formal do índice6 e os modificadores internos de cada contribuição ainda precisam ser recuperados.

## Timing inimigo

EnemySystem.IsAttackTiming37615 (`0x114d97c`,256 bytes) obtém Entity.parameter, acessa ParamSet[6] e lê Parameter.value. Este getter34149 (`0x1178ba4`,16 bytes) soma campos+0x14 e+0x1c em32 bits.

Com V como esse valor, calcula ClampMin(SDIV(FPS,V),1). Obtém Entity.enemy e lê inteiro+0x1c; retorna true se o resto de sua divisão pelo intervalo for zero, usando SDIV e MSUB. Para divisor não zero e valores normais:

```text
intervalo_inimigo = max(1, trunc(FPS / V))
momento_de_ataque = (campo_enemy_0x1c % intervalo_inimigo == 0)
```

Não aplicar a fórmula humana ao inimigo: aqui não há +100, divisão por2 nem multiplicação do FPS por100. O nome e escritores de enemy+0x1c permanecem pendentes antes de chamar esse campo de relógio/frame do ataque. O predicado isolado não prova que todo resultado true resulte em um ataque: falta conferir suas guardas e callers.

## Próxima cadeia de dano e recompensa

Inventário em `build/research/combat-calls.json`, com limites FDE e distinção de transferências diretas/indiretas:

- Damage chama Random.Hits36634 e Rand36633, GetAttributesAffinity34522, GetAttributeValue37822, Parameter.get_rate34160, Graph.Easing15901 e Parameter.Sub34170. Também contém criação de efeito e alteração de animação. Prioridade seguinte: separar chance de acerto, cálculo e aplicação, mantendo a ordem das chamadas aleatórias.
- Defeat chama EnemyData.AddDefeatCount38215, EnemyData.GetExp38212, World.CreateDrop36982 e Random.Hits100036635, além de estado/efeito. Próximo: condições de morte, quantidade/posição de drops e experiência, sem presumir recompensa garantida.

## Validação e estado

Oito FDEs de entrada em `gameplay-combat/`, seis helpers em `attack-timing-helpers/`, todos com cobertura sintática integral registrada em coverage.tsv. Inventário de chamadas para Damage/Defeat/GetTotalParamValue gerado por `tools/inspect_arm64_calls.py`. Cobertura sintática e nomes não equivalem a contratos completos: alvos exportados mas ainda não interpretados permanecem explicitamente pendentes.

Nenhum C++ alterado nem teste repetido. Próxima implementação só após fechar dados de entrada, consumo do intervalo e casos de máquina. Engenharia reversa e port continuam ativos, agora orientados ao primeiro ciclo de gameplay.
