# Fatores numéricos de dano

Data: 2026-10-25. Análise estática do mesmo APK identificado em 01. Endereços são VAs ELF; exports Ghidra acrescentam 0x100000.

Graph.Easing15901, VA0x1c5df9c, retorna início para progresso negativo e fim para progresso >= duração. No intervalo interno, converte progresso/duração para float32, acrescenta uma correção senoidal, multiplica pela diferença dos extremos, soma o início e trunca para inteiro. Multiplicação e soma são instruções separadas, sem FMA.

EnemySystem.Damage usa dois casos com amplitude zero: (10,100,50,Parameter[1].rate,0) no ramo humano não crítico e (1,30,98,wrap32(bônus-1),0) quando os atributos coincidem. No segundo caso, o fator aplicado é 100 menos o retorno. Não aplicar ambos incondicionalmente.

FastMath.Sin15096, VA0x1c3d290, converte float para double, chama ToRadians13414 em0x1b11d60 e o símbolo dinâmico sin pela PLT0x25fe590. ToRadians multiplica por double pi em0x626c28 e divide por180 em0x627428. Graph usa float pi em0x627830 e divisor314.1592712402344 em0x6278ac. Exports: build/research/damage-helpers, combat-easing e easing-radians.

Para os 148 pontos internos desses dois casos, o argumento do seno é finito; a amplitude zero elimina sua contribuição numérica. O port reproduz apenas o resultado numérico, sem simular inicialização de classes ou chamadas de biblioteca do original. Não é implementação geral de Graph.Easing. A hipótese de ambiente numérico é IEEE754 com arredondamento padrão; o estado FPCR do processo original não foi capturado.

Implementação em src/core/combat_rules.cpp preserva operações float32 separadas. O bônus menos1 usa wrap32: INT_MIN vira INT_MAX, selecionando o limite superior, não o inferior. Nenhum fator consome RNG ou altera entidades.

Verificação reproduzível: `python tools/verify_combat_easing.py` compara um modelo das operações com valores esperados independentes para os 148 pontos. A expressão inteira coincide em todos eles, mas não se generaliza para outros parâmetros de Easing. `tools/build-desktop.cmd` executa os testes C++ de todos os pontos, limites e overflow. Isso é validação contra evidência estática; ainda falta comparação com o jogo em execução.

Próximo: resolver gerador/seed e ordem de consumo aleatório, guardas de colisão, Defeat/drops e efeitos antes de integrar o pipeline completo. Ainda não existe gameplay jogável.
