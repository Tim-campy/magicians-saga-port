# Modificadores de dano e subtração do parâmetro de destino

Data:2026-10-25. Investigação estática ARM64 do APK identificado anteriormente. VAs sem base Ghidra0x100000. Nenhum APK executado ou save acessado.

## Sequência após o teste de acerto

Continuação de EnemySystem.Damage37631 em58/59. Após sucesso de Random.Hits, obtém ParamSet[0] do receptor, guardado até a chamada Parameter.Sub. A identificação nominal desse índice como HP ainda deve ser ligada ao catálogo de parâmetros; a operação e o destinatário já estão comprovados.

Receptor humano: parâmetro2 vem de MemberSystem.GetTotalParamValue, atributo do campo+0x58 da arma e bônus B de MemberSystem.GetAttributeValue. Receptor inimigo: parâmetro2 vem de EnemyData.GetParamValue, atributo do campo+0x4c dos dados e B=0. Outro receptor: parâmetro2=0, atributo4 e B=0. O atacante fornece parâmetro8 pela mesma distinção humano/inimigo/outro usada para outros parâmetros em59.

Em0x1150108–0x1150218, a sequência aritmética é:

```text
R = Random.Rand(95, 105)
I = GetAttributesAffinity(attackAttribute, atributo_receptor)
X = trunc(wrap32(R * attack) / 100) - trunc(param2_receptor / 4)
Y = trunc(wrap32(wrap32(X) * wrap32(B + 100)) / 100)
D = trunc(wrap32(ATTRIBUTE_AFFINITY_RATE[I] * Y) / 100)
C = Random.Hits(trunc(param8_atacante / 2))
```

Todas as somas/subtrações/multiplicações intermediárias relevantes são32 bits; divisões por100 usam multiplicação assinada larga por0x51eb851f e correção de sinal. Não fundir porcentagens ou adiar arredondamentos. R é o retorno do wrapper original, não uma distribuição uniforme já comprovada.

Se C=true, multiplica D por170 e divide por100 com as mesmas regras. Se C=false e atacante humano, multiplica pelo retorno de Graph.Easing(10,100,50,Parameter[1].rate,0) e divide por100. Não tratar os dois ramos como modificadores cumulativos.

Quando atributo do receptor é igual a attackAttribute, aplica ainda fator `100 - Graph.Easing(1,30,98,wrap32(B-1),0)` dividido por100. Depois escolhe mínimo aleatório: receptor humano usa Rand(1,3); receptor inimigo usa Rand(3-I,6-2*I) para I<3 sem sinal, e Rand(1,1) nos demais casos. Aplica ClampMin(D,mínimo). Outro receptor não passa por esse clamp local.

Graph.Easing15901 (`0x1c5df9c`,224 bytes) foi exportado: calcula em float32, com ramo de seno e limites/conversão final. Os callers aqui passam último parâmetro0, mas não foi substituído por interpolação inteira: a ordem das operações float e casos extremos ainda precisa ser validada. Fórmula completa de Damage continua parcial, incluindo efeitos posteriores à subtração.

## Afinidade e guardas

AppData.GetAttributesAffinity34522 (`0x11177c8`) lê a linha da tabela estática+0x440 pelo atributo atacante. Se primeiro elemento coincide com receptor, retorna0; se segundo coincide, retorna2; sem coincidência, se qualquer dos dois elementos é-1 retorna3, senão1. Não é uma comparação direta de igualdade entre atributos. A tabela de taxas+0x448 foi identificada por metadata/tabela de offsets como ATTRIBUTE_AFFINITY_RATE; valores concretos ainda não recuperados.

Antes de Parameter.Sub, lê MyConfig.DEBUG (campo estático+0x4). Quando DEBUG=true e atacante não humano, pula essa subtração. Isso não equivale a desativar a função inteira: o fluxo segue para efeitos posteriores. Valor efetivo de DEBUG ainda precisa ser confirmado; não fixar com base no nome.

## Parameter.Sub: implementação comprovada

Parameter.Sub34170 (`0x1178dec`,64 bytes) e IntExtension.Clamp33949 (`0x10eda50`,28 bytes) estabelecem:

```text
candidato = wrap32(value_ - amount)
novo = candidato
se maxValue_ != 0:
    se candidato < 0: novo = 0
    senão se candidato > maxValue_: novo = maxValue_
value_ = novo
retornar wrap32(candidato - novo)
```

Tabela de offsets liga value_ a+0x14, maxValue_ a+0x18, extraValue_ a+0x1c e extraMaxValue_ a+0x20. Sub não usa os extras. Máximo0 desliga o clamp, permitindo valor negativo; retorno de excesso inferior é negativo. O clamp testa limite inferior antes do superior, inclusive quando máximo é negativo. Não substituir por std::clamp com limites inválidos.

`src/core/combat_rules` agora oferece subtract_parameter com resultado e resto, sem acoplamento a ECS. Testes acrescentados: dano comum, excesso, máximo0, amount negativo, máximo negativo e wrap32. Build Windows Release e CTest **9/9 passaram** via tools/build-desktop.cmd; log `build/research/parameter-sub-build.log`. Testes são derivados do assembly, não comparação dinâmica com o jogo.

## Próximo recorte

Recuperar valores das tabelas de afinidade e nomes dos parâmetros; fechar Easing e efeitos após Sub (recuo, animações, retorno). Só então compor um pipeline de dano nativo. A rotina de subtração está portada; isso não constitui combate integrado nem morte/recompensa completa. Gameplay permanece prioritário.
