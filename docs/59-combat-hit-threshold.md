# Acerto no combate: limiar e consumo de RNG

Data:2026-10-25. Análise estática do ELF ARM64 identificado em58; VAs sem base Ghidra. Nenhum APK executado.

## Argumentos e fórmula comprovada

Metadata de EnemySystem.Damage37631 nomeia seus quatro argumentos como attacker, receiver, attack e attackAttribute. Isso associa x20 ao atacante e x21 ao receptor no corpo de0x114fc44, sem deduzir papéis só pelos nomes de variáveis do decompilador.

Entre0x114fd10 e0x114fe94, busca parâmetro9 do receptor e parâmetro7 do atacante. Para humano usa MemberSystem.GetTotalParamValue; para inimigo EnemyData.GetParamValue com índice e EnemyComponent+0x10; para entidade sem esses dois componentes usa0. Se possuir ambos, o ramo humano tem precedência. O significado nominal desses índices e do campo+0x10 ainda deve ser recuperado.

```text
base = atacante humano ? 90 : 80
ajuste = clamp(wrap32(param7_atacante - trunc(param9_receptor / 2)), -60, 15)
limiar = base + ajuste
acerto = Random.Hits(limiar)
```

AppData.Clamp34248 (`0x110180c`,20 bytes) faz clamp assinado. CINC/ASR no cálculo da metade implementa truncamento para zero; SUB é32 bits. O limiar humano fica entre30 e105 e o dos demais entre20 e95. Não aplicar clamp adicional em100 nem chamar esses números de probabilidades uniformes medidas.

Random.Hits36634 (`0x1128f34`) sempre chama Rand(100), compara retorno<limiar e retorna o booleano. Mesmo limiar105 não evita consumo aleatório. O ramo de falha cria efeito via World.CreateEffect e retorna0 antes de obter o parâmetro0 de destino ou entrar no cálculo seguinte. O ramo de sucesso prossegue para modificadores e aplicação de dano; a fórmula completa ainda está em análise.

## Wrappers aleatórios

Random.Rand36632 (`0x1128e00`) chama JRandom.NextInt, faz negação condicional em32 bits se negativo e resto por SDIV/MSUB. INT32_MIN continua negativo após essa negação; não substituir cegamente por abs de C++ nem assumir faixa0–99 para toda saída possível. O algoritmo e o domínio real do gerador ainda não foram estabelecidos.

Random.Rand36633 (`0x1128ea8`,dois argumentos) usa NextInt diretamente e calcula resto por wrap32(max-min+1), somando min. No caminho de dano confirmado após acerto, recebe95 e105 em0x1150108–0x1150114. Não atribuir distribuição uniforme antes de resolver NextInt.

JRandom.NextInt13183 (`0x1b04d9c`) escolhe objeto nos campos+0x10/+0x18/+0x20 conforme inteiro de configuração e despacha slot5, class+0x188. Algoritmos concretos, seleção e seeds continuam pendentes. Esta descoberta impede implementar RNG substituto e alegar determinismo fiel.

## Implementação e teste

`src/core/combat_rules.hpp/.cpp` implementa apenas hit_threshold, recebendo parâmetros já resolvidos e flag de atacante humano. Mantém divisão truncada, wrap32 explícito da subtração e limites originais. Não resolve equipamentos, não executa Random.Hits nem aplica HP.

`tests/test_combat_rules.cpp` verifica bases distintas, extremos do clamp, pontos internos, metade negativa, limiar105 preservado e wrap de INT32_MIN/MAX. Fixtures derivadas do assembly, não capturas de jogo. `tools\build-desktop.cmd`: build Windows Release e CTest **9/9 passaram**, log `build/research/combat-rules-build.log`. Linux não testado nesta rodada.

Evidência adicional: `combat-hit/` e `combat-rng/` com cinco FDEs integralmente exportados; Damage relido de58. Próximo: recuperação de defesa/atributos e multiplicadores, mantendo ordem de chamadas aleatórias; gerador concreto e origem dos parâmetros também abertos. Ainda sem gameplay integrado ou port jogável.
