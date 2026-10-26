# Preparação nativa dos dados de fala e evento

Data: 2026-10-25. Capítulo 4 — gameplay/M3; preparação de dados, sem interface visual integrada.

## Evidência e contrato

Continuação dos relatórios [85](85-presentation-text-preparation.md), [87](87-native-presentation-split.md) e [88](88-native-presentation-arguments.md). Endereços são VAs ELF ARM64 do artefato identificado no inventário.

| Método original | VA | Contrato utilizado |
| --- | --- | --- |
| SubForm.MakeTalkForm, índice36177 | 0x1204bc4 | Substitui argumentos quando não nulos, separa páginas, inicializa tipo0, coordenadas/seleção0, índice de texto0 e próximo índice-1. |
| SubForm.MakeEvent, índice36179 | 0x1205120 | Substitui argumentos, separa por TAB e depois &, interpreta campo0 antes de acessar campo1, ignora campos posteriores. Inicializa tipo4 e quatro inteiros de animação em zero. |
| SubForm.MakeEvent, índice36178 | 0x1205028 | Overload externo constrói type + '&' + text; resolução do catálogo ainda não integrada. |

Evidência local: `build/research/form-data-preparation/M36177.asm` e `build/research/delay-attach-more/M36179.asm`. Campos SubForm: type+0xe8, eventIds+0xf0, eventAnim+0xf8, eventTexts+0x100, select+0x13c, x/y+0x150/154, textAry+0x1f8, character+0x200, textIdx+0x210, args+0x218 e nextIndex+0x220.

Confiança: **confirmed** para esse recorte de fluxo estático e campos. Não estabelece comportamento em execução.

## Implementação

`src/core/gameplay/presentation_forms.*` compõe os helpers UTF-16 existentes e retorna dados preparados. Falas nulas/vazias não recebem página inventada: a ausência do primeiro elemento produz erro. Uma entrada não vazia composta somente de espaços pode produzir uma página vazia. Eventos nulos/vazios podem produzir vetores vazios.

Conversão numérica exige `PresentationFormHost::parse_event_type`; cultura e semântica completa de Int32.Parse não são presumidas. Tradução e configuração continuam explícitas no host. Exceções C++ representam entradas inválidas, sem alegar equivalência de tipos de exceção gerenciada.

O modelo preserva valores; não replica identidade de strings gerenciadas, alocação de SubForm ou efeitos de TextLayout. O original configura TextLayout, tamanho196×60, velocidade1500 e outros campos antes de apresentar a primeira página; suas unidades e comportamento ainda precisam ser reconstruídos.

## Validação

Comando: `tools\build-desktop.cmd`. Windows: **16/16 testes CTest aprovados**, log local `build/research/presentation-forms-build.log`.

Novo teste cobre campos iniciais, páginas vazias, ordem de parsing/erros e integração executor → opcode8 → espera10 → opcode12 → preparação. O catálogo e o tradutor do teste são fixtures explícitas; não constituem integração com os registros reais.

## Limites e próximo passo

Sem renderização, entrada, avanço de páginas, tradução real ou comparação dinâmica. Próximo passo: ligar catálogos e configuração comprovados ao host e reconstruir TextLayout/fluxo de apresentação. O primeiro loop jogável permanece pendente.
