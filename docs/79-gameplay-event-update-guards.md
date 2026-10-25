# Guardas da atualização de eventos de gameplay

Data: 2026-10-25. Análise estática ARM64; endereços abaixo são VAs ELF, sem a base adicional 0x100000 do export Ghidra. Nenhuma execução do APK.

## Evidência

SelectForm.Update35965, export em `build/research/delay-resume/M35965.asm`, testa o campo +0xc0 em 0x11ebde8. Se não nulo, limpa esse campo e retorna antes da fila de DelayEvent. O significado desse campo ainda não foi resolvido.

O despacho por +0xf8 em 0x11ebe1c separa caminhos de estado. No caminho que chega a 0x11ec49c, UpdateNotifyValue35967 é chamado antes de TapjoyForm.AcquireBalance35255. Se este retorna verdadeiro, o Update retorna imediatamente, antes de decrementar esperas. Portanto não se pode associar cada chamada de Update a uma passagem da fila.

AcquireBalance, VA0x11b72a4, foi exportado em `build/research/delay-attach-more/M35255.asm`. Se o inteiro +0x5c do objeto obtido em 0x1185ef4 for menor que1, retorna falso. Caso contrário, percorre formulários do FormManager. Um formulário compatível com o tipo consultado, com campo +0xe8 igual a0, também leva ao retorno falso. Se não encontrar esse caso, aloca o formulário, chama seu construtor e o insere no manager, retornando verdadeiro. A origem e unidade do inteiro +0x5c ainda precisam de rastreamento; o nome AcquireBalance não comprova acesso de rede nesse corpo.

Após essa guarda, SelectForm chama SetActiveX35990, UpdateDecoObject36003 e UpdateTownHuman35998. O teste de +0xd0 >2 em 0x11ec4e0 pula a seção inicial de eventos/guias, mas seu destino é 0x11ec570, imediatamente antes da fila de DelayEvent. Não é uma guarda que desabilita essa fila. Efeitos transitivos dessas chamadas e outros estados continuam pendentes.

## Continuação associada ao formulário

AppData.Push34382 (VA0x110d3b8; `build/research/delay-attach/Attach.asm`) obtém FormManager.GetInstance35754, consulta GetCurrentForm19699 e encaminha ao overload Push19667. Não há atribuição direta ao campo SubForm+0x1d0 nesse wrapper. A investigação de quem escreve esse campo permanece aberta.

Os corpos SetEvent36190 e MakeEvent36179 também foram exportados nesta rodada; não apresentam gravação direta em +0x1d0. Isso não exclui atribuições transitivas, indiretas ou outros overloads.

## Impacto e próximo recorte

Prioridade solicitada: gameplay. Seguir o produtor da continuação +0x1d0 e os consumidores de hour_, depois portar apenas o contrato de execução que estiver fechado. Em seguida, integrar combate, coleta e concessão de recompensas já documentados a uma primeira simulação determinística, antes de expandir apresentação gráfica.

Não houve alteração C++ nem repetição de testes nesta rodada. O último build registrado permanece Windows/CTest10/10 (relatório70); isso valida módulos isolados, não gameplay integrado. Ainda há investigação estática útil e nenhum teste dinâmico é exigido agora.
