# Derrota e ordem das recompensas

Data: 2026-10-25. Evidência estática ARM64 do APK inventariado em01. VAs ELF, sem acréscimo Ghidra0x100000. Nenhum jogo executado.

## Sequência de EnemySystem.Defeat

Método37632, VA0x1150b98, 1040 bytes. Export existente em build/research/gameplay-combat/M37632_EnemySystem_Defeat.asm; helpers novos em build/research/defeat-helpers, todos com cobertura sintática integral registrada em coverage.tsv. Offsets obtidos da tabela IL2CPP estão em defeat-field-offsets.json.

1. Zera vx/vy/vz de SpeedComponent. Chama FlyAway(entidade,-100,100,-100,-50,1), depois ChangeState(entidade,4).
2. Grava ColorComponent como quatro inteiros [255,0,255,0]. Não interpretar isso como RGBA normalizado nem substituir por branco sem estudar o consumidor.
3. Chama EnemyData.AddDefeatCount(1). Em seguida incrementa EnemySystem.defeatedEnemiesNum_ (+0x34) somente se EnemyData.Check(1) for falso. São contadores diferentes.
4. Atualiza remainingEnemiesNum_ (+0x68) para max(wrap32(anterior-1), (~EnemyComponent.Check(1))&1). O Check do componente usa flag+0x14 e delega a BitUtil.Check13688; o Check de BaseData testa diretamente máscara1 em+0x1c. Não confundir os dois objetos de flags. Nome semântico do bit ainda precisa de confirmação.
5. Consulta GetExp com EnemyComponent.level (+0x10). Se resultado >0, calcula Graph.Easing(25,50,500,exp,-50) e cria drop tipo0, valor exp, posição (x-5,y,z-10), usando o retorno como argumento de escala. Depois grava DepthComponent.extra=-10.
6. Examina o primeiro grupo de EnemyData.treasureDrop_ (+0x40). Para cada linha em ordem, lê dois inteiros (id,limiar), chama Hits1000(limiar) e continua somente quando falha. No primeiro sucesso cria drop tipo3 com id, posição(x,0,z), argumento25 e extra de profundidade -10; depois termina. Não há sorteios para as linhas seguintes.

Nulls e arrays curtos têm caminhos de exceção; não convertê-los silenciosamente em ausência de recompensa. O corpo não contém uma guarda local de “já derrotado” antes das mutações acima. Isso não prova que chamadas repetidas sejam possíveis: guardas dos callers/estados ainda precisam ser examinadas.

## Helpers e consequências para fidelidade

EnemyData.AddDefeatCount38215, VA0x116eaf4, soma em32 bits antes de Clamp(0,INT_MAX). Portanto INT_MAX+1 dá zero após overflow e clamp; não é incremento saturado em INT_MAX.

EnemyData.GetExp38212, VA0x116e9c0, lê os dois primeiros inteiros de exp_ (+0x38) e chama Easing(início,fim,999,wrap32(level-1),0). Não arredondar com interpolação inteira sem verificar os limites e valores concretos; o caso genérico tem conversões float32 e diferença dos extremos em32 bits.

Random.Hits100036635, VA0x1128f94, sempre chama Rand(1000) e compara retorno < limiar. Não elimina consumo para limiar0 ou1000. O Rand já investigado tem tratamento próprio de sinal/INT_MIN; o nome do helper não prova distribuição uniforme nem probabilidade matemática exata.

World.CreateDrop36982, VA0x1135298, cria uma entidade e componentes. A tabela de salto em0x627b94 contém[30,0,6,11], relativa a0x1135320 em unidades de4 bytes. Tipo0 entra em0x1135398 preservando a escala recebida. Tipo3 entra em0x113534c, consulta dados pelo id e **sobrescreve a escala com100**, apesar do caller passar25. Logo o argumento25 não pode ser tomado como escala final de tesouro. A função retorna a entidade; concessão efetiva ao jogador depende de coleta e ainda não foi mapeada.

## Estado e próximo passo

Fluxo de seleção de recompensas confirmado estaticamente; nenhum código de gameplay foi acrescentado neste ciclo para evitar integrar coleta e RNG ainda desconhecidos. Sem alteração C++, testes de build não foram repetidos. A última execução permanece9/9, no relatório64.

Próximo: consumidores de DropComponent e guarda de coleta, identidade dos flags de inimigo, backend/seed aleatório e estados que impedem derrota duplicada. O Easing com amplitude-50 usado para apresentação do drop não está coberto pelos dois casos numéricos portados em64. Ainda há avanço estático útil antes de exigir testes dinâmicos.
