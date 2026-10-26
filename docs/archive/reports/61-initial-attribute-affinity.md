# Tabelas iniciais de afinidade e implementação nativa

Data:2026-10-25. Gameplay prioritário. Análise estática do ELF ARM64/metadata do APK1.4.0 identificado nos relatórios anteriores. Nenhuma execução Android.

## Origem comprovada

AppData..cctor34526 (`0x1117b30`,11.308 bytes) foi exportado com cobertura sintática integral para `build/research/appdata-constants/`. Investigação delimitada aos trechos que constroem as tabelas; o restante do inicializador não foi integralmente interpretado.

Entre0x1119ef8 e0x111a044, aloca array externo de cinco posições e arrays internos de dois inteiros. Stores explícitos, juntamente com zeros de arrays recém-alocados, estabelecem pares:

```text
índice0: [2, 1]
índice1: [0, 2]
índice2: [1, 0]
índice3: [-1, -1]
índice4: [-1, -1]
```

O store0x111a044 guarda o array em campos estáticos+0x440, consultados por GetAttributesAffinity34522 (60).

Em0x111a058 aloca array de quatro inteiros, inicializa via RuntimeHelpers.InitializeArray8401 (`0x1d891c4`) no site0x111a068 e guarda em+0x448 no site0x111a078. GOT0x272ff30 resolve para referência codificada0x800000c3: field-ref97, campo24324 de PrivateImplementationDetails. Registro field_defaults aponta ao offset relativo187880 de default_data da metadata. Os16 bytes little-endian são:

```text
5a000000 2d000000 14000000 3c000000
90       45       20       60
```

SHA-256 desses bytes: `a83b4f7b8cdb34a9cb47e027d6371b115af144c8036f48b3bc1625d52cd7f339`, exatamente o identificador do campo estático. Resultado estruturado local em `build/research/affinity-initial-values.json`. São taxas iniciais; não foi provada ausência de alterações posteriores.

## Matriz derivada

Com o seletor de afinidade recuperado em60, as taxas resultantes são:

| Atacante / receptor |0|1|2|3|4|
|---|---:|---:|---:|---:|---:|
|0|45|20|90|45|45|
|1|90|45|20|45|45|
|2|20|90|45|45|45|
|3|60|60|60|60|60|
|4|60|60|60|60|60|

Não atribuir ainda nomes de elementos a esses índices. O caller de Damage usa a taxa como multiplicador seguido de divisão por100, na posição documentada em60; não é a taxa de acerto de59. A tabela não resolve sozinha a fórmula completa do dano.

Casos fora da matriz também importam: receptor não indexa array, apenas é comparado. Receptor-1 com atacante3/4 coincide com o primeiro elemento e retorna seletor0/taxa90, antes da regra de sentinela. Atacante negativo ou>=5 causa limite de array; não aplicar fallback genérico.

## Implementação e validação

`src/core/combat_rules` agora contém AttributeAffinityTables, fábrica dos valores iniciais e attribute_affinity. Tabelas são estado explícito recebido pela função, permitindo incorporar alterações se forem comprovadas; não foram fixadas como configuração imutável global. Ordem preservada: primeiro par→0, segundo→2, qualquer sentinela-1→3, demais→1.

Teste ampliado com matriz esperada das25 combinações, precedência de sentinelas e entradas duplicadas, receptor fora da faixa e rejeição dos dois limites de atacante. `tools\build-desktop.cmd`: build Windows Release e CTest **9/9 passaram**, log `build/research/affinity-build.log`. Sem teste Linux ou comparação dinâmica nesta rodada.

Próximo: recuperar nomes/origem dos parâmetros e fechar Graph.Easing/efeitos após a subtração para integrar o cálculo completo. Não há combate jogável ainda; o trabalho aproxima o núcleo dos dados concretos desta versão.
