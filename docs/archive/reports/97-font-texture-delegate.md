# Delegate de métricas por textura e propagação da seleção

Data: 2026-10-25. Capítulo4/M3. Investigação estática; nenhuma rota executada.

## Resolução do delegate

O GOT0x2740698 usado por KairoPlugin.GetFontTextureSize14837 resolve descriptor de método15013, <>c__DisplayClass151_0.<GetFontTextureSize>b__0, VA0x1c3683c. O objeto capturado guarda texto+0x10 e tamanho+0x18.

No delegate, Config.UNITY_EDITOR (offset estático0x30) seleciona o caminho por reflexão. Fora dele, Config.APPLI_TYPE (offset estático0) é comparado com2; valores diferentes retornam null nessa rota. Não se deve transformar a existência de ambos os blocos em prova de execução de ambos no APK.

### Caminho de editor

Literais confirmados nos metadados:
- GOT0x2740ac8: WinLib.
- GOT0x2740ab8: kairo.windows.plugin.Utility.
- GOT0x2740ae8: GetFontTextureSize.

O caminho carrega assembly/tipo e invoca o método por reflexão, passando texto/tamanho. Nenhuma implementação WinLib foi localizada ou instalada neste ciclo. Esse caminho presente no IL2CPP não constitui uma dependência Windows entregue ao port.

### Caminho APPLI_TYPE==2

GOT0x2740ae0 contém getFontTextureSize. GOT0x2740aa0 resolve method-spec57142: definição59748, AndroidJavaObject.CallStatic, instância genérica192. O argumento genérico é SZARRAY cujo elemento é U1, ou seja byte[].

A chamada ocorre em0x1c36b3c sobre o objeto estático KairoPlugin+0x10, com array de argumentos contendo texto e inteiro de tamanho. Isso fecha a fronteira Java dessa medição; ainda falta rastrear a classe Java concreta e implementação chamada.

### Resultado

Resultado não nulo é encapsulado em ByteArrayInputStream13499 (0x1be8544), seguido por duas chamadas StreamUtil.ReadFloat14446 (0x1c0c764), fechamento do stream e construção de SizeF16332 (0x1c639e8). Assim, a rota retorna uma dimensão obtida do payload binário; não é simples contagem de caracteres.

Não declarar encoding/endian desse payload apenas pelos nomes. O decoder ReadFloat precisa ser conferido caso essa rota seja usada no desktop.

## Font.useFontTexture_

Foram encontrados dois stores byte+0x48 nos métodos do tipo Font:
- _set17764 (0x1b78bf8), escrita0x1b78c3c: copia o flag da fonte de entrada antes de atualizar GUIStyle/tamanho/estilo.
- _copy17793 (0x1b7a62c), escrita0x1b7a650: copia o mesmo flag junto dos demais atributos.

O construtor17759 e _init17761 foram inspecionados; _init escolhe fonte padrão, GUIStyle e configura tamanho/estilo. Não identificada nesses trechos uma atribuição explícita true ao flag. Isso não prova que nunca seja ativado: chamadas subsequentes, escritas de outras larguras, aliases, configuração externa e reflection ainda não estão cobertos.

A varredura de STRB imediato+0x48 em segmentos executáveis foi usada somente como lista de candidatos, com dono por FDE. Offset coincidente em outro tipo não prova acesso a Font; ausência nessa varredura não prova ausência de escrita.

## Evidências e validação

Artefatos locais:
- build/research/font-backend/M15013.asm e construtores/Set.
- build/research/font-selection/F1b78bf8.asm e F1b7a62c.asm.
- build/research/font-init-body/M17761.asm.
- build/research/font-store-candidates.json, limitado a candidatos de KairoLibrary.

Descriptors, tipos e method-specs conferidos no ELF/metadados inventariados. Cobertura de disassembly continua sintática, sem alegar cobertura dinâmica. Nenhuma mudança C++ nem testes de produto repetidos; último build18/18 permanece o de95.

Confiança **confirmed** para delegate, ramos, chamada Java e cópia do flag; seleção efetiva da fonte nas falas **unknown**. Próximo: resolver inicialização/configuração do flag e classe Java; comparar isso com a fonte realmente criada no boot antes de escolher a rota de reconstrução.
