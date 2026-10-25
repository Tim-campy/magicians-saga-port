# Retomada dos eventos adiados

Data: 2026-10-25. Evidência estática ARM64, VAs sem base Ghidra0x100000. Nenhuma execução do jogo.

## Consumidores encontrados

Busca BL/B para GetDelayScript34383 encontrou SelectForm.Update35965 em0x11ec6b0 e SubForm.Finish36168 em0x1204914. Registro em build/research/delay-script-callers.json. Busca direta não prova exclusividade. Corpos exportados em delay-resume, junto ao construtor DelayEvent35499 e GetDelayScript.

Campos confirmados na metadata/offsets: delayId_+0x10, line_+0x14, wait_+0x18, hour_+0x1c, talkChara_+0x20, args_+0x28, argi_+0x30. O construtor copia argi_ para um novo array quando não nulo; não mantém o array original por referência. args_ é armazenado diretamente.

## Passagem de SelectForm.Update

No trecho0x11ec578–0x11ec6d8, obtém a contagem da coleção AppData+0x200 uma vez, depois percorre índices. Somente hour_==-1 entra no decremento: wait_=wrap32(wait_-1). Se o resultado>0, mantém evento. Caso contrário:

1. Remove a entrada pelo índice antes da execução da continuação.
2. Converte talkChara_ por SecureInt e grava no elemento7 do array em AppData+0x198 → +0x20.
3. Decrementa índice e limite local de iteração para compensar a remoção.
4. Resolve GetDelayScript(delayId_) e chama ExecuteScript com line_, args_ e argi_ guardados.

A contagem local não é renovada após a continuação. Novos eventos acrescentados ao fim não ampliam essa passagem. Outros efeitos de callbacks sobre a lista precisam ser considerados na integração; não substituir o laço por enumeração que proíba toda mutação.

A espera é medida em passagens alcançadas desse trecho. Não foi demonstrado que equivale a segundos, frames visuais ou todo Update: guardas anteriores do formulário e outros consumidores de hour_ ainda precisam ser mapeados. Um wait_ inicial0 torna-se-1 e dispara; INT_MIN decrementado vira INT_MAX e não dispara nessa passagem.

## Retomada ao fechar formulário

SubForm.Finish chama primeiro o Finish da base. Se o campo de continuação+0x1d0 estiver presente, restaura talkChara_ no **elemento5**, não7, do mesmo array de estado; então resolve delayId_ e retoma de line_ com args_/argi_. Não decrementa wait_ nesse corpo. A atribuição da continuação ao formulário e a razão dos slots diferentes permanecem pendentes. Não corrigir essa assimetria por preferência de projeto.

## Resolução de script

GetDelayScript em0x110d6dc retorna null para delayId_>999. Para valores<=999, calcula resto assinado por1000, indexa Data.eventData e retorna execsScript_. Não há guarda antecipada para negativos: resto negativo falha na checagem de índice, exceto múltiplos negativos de1000 que resultem em0. Null no registro também falha. Portanto não generalizar o contrato tolerante de ExecEvent para essa resolução.

## Estado e próxima ação

O ciclo anterior foi progresso ao identificar opcode10 e suas dependências. Neste ciclo a continuação foi ligada a dois consumidores concretos, ordem de remoção e contratos distintos de estado. Sem C++ alterado nem testes repetidos; último build nativo permanece10/10 em70. Ainda faltam guardas de atualização, eventos com hour_ diferente de-1 e atribuição ao formulário antes de portar scheduler completo. Há trabalho estático útil.
