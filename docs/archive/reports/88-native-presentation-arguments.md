# Substituição nativa dos argumentos de apresentação

Data: 2026-10-25. Contratos de StringUtil.Replace14531/14532 recuperados do ELF; sem execução do APK.

## Correção de evidência anterior

O relatório85 inverteu a condição de SanSanRemove. A instrução em VA0x1c18f60 é `cbz w8,0x1c18fac`: **false pula as verificações/limpeza por regex; true permite esse caminho**. A redação de85 foi corrigida com referência a este relatório. Nenhum código nativo anterior implementava essa condição; a nova implementação usa a leitura conferida e testes para ambos os valores.

Com a propriedade true, a limpeza só ocorre se nem o texto anterior à substituição nem o argumento traduzido já correspondem ao padrão `(さん(</co>)?)さん`. A ordem e o curto-circuito são relevantes, inclusive quando a tradução retorna null.

## Implementação

O módulo existente `presentation_text` ganhou `substitute_form_arguments`, trabalhando com valores UTF-16 opcionais:

1. Texto null, args null ou args vazio retornam o texto sem consultar o host.
2. Args não vazio é dividido por TAB sem trim nem exclusão de campos vazios.
3. Para cada campo, chama o tradutor do host, correspondente a Language.TranslateText(campo,false), e só então consulta SanSanRemove.
4. Substitui literalmente todas as ocorrências de `<índice>`; a busca continua depois do texto inserido. Um marcador do índice atual inserido pelo argumento não é reprocessado, mas índices posteriores podem alcançá-lo em suas próprias passagens.
5. Quando habilitada pelas guardas, aplica uma única passagem de substituição do padrão de honorífico pelo grupo1.

O padrão fixo é reconhecido diretamente sobre unidades UTF-16, preservando a parte `さん` e o fechamento opcional `</co>`. A implementação usa correspondências da esquerda para a direita, sem sobreposição ou repetição até estabilizar; não é uma implementação geral de regex.

Tradução null deixa o texto inalterado no caminho sem checagem do argumento. Com limpeza habilitada e sem correspondência preexistente no texto, o teste do argumento null falha, como o acesso a Regex.IsMatch nesse caminho. Exceções C++ representam essas falhas de contrato; não reproduzem tipos/mensagens do runtime .NET.

O host deve implementar tradução e leitura da configuração reais; não há tradutor identidade embutido no módulo. A configuração é consultada depois de cada tradução, evitando congelar um valor que possa mudar durante callbacks. A API preserva valores e distinção null/vazio, não identidade de referências de strings gerenciadas ou suas alocações.

## Testes e integração

Teste novo `presentation_arguments_contract` verifica:

- Retornos antecipados, ordem traduzir→ler configuração, campos vazios e espaços nos argumentos.
- Substituição repetida, índices com mais de um dígito, ausência de recursão no marcador atual e encadeamento para marcador posterior.
- Condição true/false, tag de cor, duplicações preexistentes, múltiplas correspondências e uma passagem não recursiva para três honoríficos consecutivos.
- Tradução null, curto-circuito e mudança de configuração durante o tradutor.
- Sequência real de preparação: substituir primeiro, depois dividir/trim. Um TAB introduzido pelo tradutor passa a separar segmentos do formulário.

`tools\build-desktop.cmd`: **15/15 testes CTest passaram em Windows**, sem warnings de compilação encontrados no log `build/research/presentation-arguments-build.log`. Testes usam tradutores controlados para observar o contrato; não verificam o serviço de localização do jogo.

## Estado e próximo passo

Split, trim e substituição de argumentos estão implementados como etapas nativas. Ainda faltam tradução configurada, catálogo/estado de execução, modelo dos formulários e renderer para uma tela fiel. Próximo: ligar a preparação às estruturas de TalkData/EventMsgData e fechar os caminhos de tradução necessários, sem assumir língua ou estado de SanSanRemove. Não houve teste gráfico, Linux ou execução dinâmica do APK.
