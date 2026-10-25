# Stamina e encaminhamento dos eventos de apresentação

Data: 2026-10-25. Evidência estática ARM64, VAs sem base adicional Ghidra0x100000. Nenhuma execução do APK ou modificação de estado de compra.

## Limite de stamina

GetStaminaMax34465, VA0x110d3fc, obtém MyBilling.GetInstance35265, chama BillingManager.GetItem35372 com a variável estática MyBilling+0x1c e lê BillingItem.GetState35338. Metadata e offsets identificam a variável como ITEM_STAMINA_UP (field21703/type4228).

O resultado é10 quando o estado lido é zero e20 quando é diferente de zero. O método não testa especificamente um enum “comprado” nem positividade. Ausência de instância/item causa falha; não existe fallback10 para objeto null no corpo analisado. GetState em0x11bc1dc lê o objeto+0x48 e encaminha à conversão de SecureInt.

`stamina_limit_from_item_state` implementa somente essa decisão sobre o estado decodificado. Não obtém, concede, altera ou assume o estado do item. A origem e manutenção do estado continuam responsabilidade da integração fiel com os dados do original.

## Opcode31

No dispatcher34379, trecho0x110cefc–0x110cfd8:

1. Consulta GetStaminaMax antes de validar a presença do argumento percentual.
2. Obtém o saldo SecureLong de AppData+0x190 → array+0x28, elemento1.
3. Calcula `delta = trunc_zero(wrap32(percentual * primeiro_limite) / 100)`.
4. Decodifica o saldo e consulta GetStaminaMax novamente.
5. Soma delta estendido com sinal ao saldo, com wrap64; limita ao segundo máximo e depois aplica piso zero.
6. Grava o resultado no slot de stamina e retorna null.

A instrução de comparação estende o segundo limite com zeros para64 bits. O getter observado só retorna10/20; a implementação preserva a largura mesmo assim. Não reaproveitar a fórmula de moeda/cristais nem arredondar o percentual para o inteiro mais próximo.

## Opcodes8 e12

O dispatcher chama respectivamente SubForm.MakeTalkForm36175 (VA0x1204abc) e MakeEvent36178 (VA0x1205028), passando ID resolvido e o objeto args. Depois chama AppData.Push34382 com o formulário retornado e devolve esse mesmo objeto como resultado do opcode. Não foi encontrada uma espera implícita no executor da matriz após esses opcodes; ela continua até terminar ou atingir opcode10.

Exports adicionais mostram a fronteira que ainda falta reconstruir:

- MakeTalkForm consulta Data.talkData[id], lê o inteiro+0x20 do registro e busca texto em um array estático AppData+0x378; encaminha ambos e args ao overload36177.
- MakeEvent consulta Data.eventMsgData[id], combina o inteiro+0x20 e o conteúdo+0x28 em uma string e encaminha ao overload36179. O significado completo dessa composição e dos campos ainda precisa ser fechado antes de produzir telas reais.

Os novos callbacks de apresentação não substituem essas consultas por texto inventado. Devem ser implementados pelo host conforme a investigação dos formulários e recursos.

## Implementação

`src/core/gameplay/event_dispatch.{hpp,cpp}` liga o executor aos efeitos numéricos e às fronteiras de apresentação. Trata os opcodes encontrados no Event.txt extraído:1/2/4/8/10/12/15/31. Para1/2/4/15 usa o módulo de progressão; para8/12 exige criação e push explícitos; para31 exige leituras/escrita de stamina em ordem. Os demais retornam “não tratado”, evitando declarar que todo o dispatcher original foi reconstruído.

No dispatcher unitário, opcode10 não agenda nada; o executor de matriz o intercepta antes, como documentado em77/82. Null/linha vazia são tolerados no unitário, enquanto o executor da matriz falha ao encontrar esses elementos. A substituição de argumentos ocorre antes do switch, sem alterar a linha original.

`EventEffectsHost` é uma fronteira de integração, não uma implementação de UI, save ou billing. O acesso `progression()` fornece o estado decodificado estável. O módulo não replica inicializadores estáticos, falhas de alocação ou armazenamento protegido do runtime.

## Verificação e limites

`tools\build-desktop.cmd`: **13/13 testes CTest aprovados em Windows**. Log local: `build/research/event-dispatch-build.log`.

Novo teste cobre limite zero/não zero, truncamento positivo/negativo, wrap32/64, clamp, duas consultas distintas de limite, ordem de efeitos em percentual ausente, criação/push/retorno do formulário e propagação de falha de push. Uma sequência integra dinheiro, fala, espera, stamina e formulário de evento com os módulos reais; callbacks de UI apenas observam chamadas, não desenham telas.

Linux, execução gráfica e comparação com o APK continuam não realizados. Próximo recorte: decodificar a preparação de dados dos formulários e integrar catálogo/identidade/estado inicial sem inventar conteúdo, condição de compra ou política de atualização.
