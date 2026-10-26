# Medição Java de texto e ligação com o leitor nativo

Data: 2026-10-25. Capítulo 4/M3. Evidência estática do APK inventariado; nenhuma execução Android.

## Classe concreta

KairoPlugin.Init14724, VA0x1c22a6c, testa Config.APPLI_TYPE==2 e UNITY_EDITOR==false antes de criar três AndroidJavaClass. O literal no GOT0x27402a8 é `kairo.android.plugin.Utility`; a chamada ao construtor59778 (0x238a4a8) ocorre em0x1c22c78 e o objeto é escrito no campo estático+0x10 em0x1c22c84. É o campo consumido pelo delegate15013 descrito no relatório97.

Os outros dois literais desse bloco são `kairo.android.plugin.Config` (GOT0x2740298, campo+0x8) e `android.os.Build$VERSION` (GOT0x27402a0, campo+0x18). Isso identifica a classe chamada, mas não prova os valores efetivos da configuração durante uma partida.

## Contrato no DEX

Método exato: `kairo.android.plugin.Utility.getFontTextureSize(Ljava/lang/String;I)[B`. A inspeção smali confirma esta sequência:

1. Criar Paint; ativar linearText.
2. Converter o tamanho inteiro recebido para float e passá-lo a setTextSize.
3. Selecionar Typeface.SANS_SERIF e ativar antiAlias.
4. Obter largura com measureText(String).
5. Obter a segunda medida pelo retorno float de getFontMetrics(null).
6. Escrever largura e segunda medida, nessa ordem, com DataOutputStream.writeFloat em ByteArrayOutputStream; obter o array, fechar DataOutputStream e retornar.

O método não mede altura por limites dos pixels do texto e não seleciona uma fonte extraída do APK. A face concreta, fallback de glifos e valores produzidos pelo Paint não foram observados. A presença de fontes nos assets não permite equipará-las a SANS_SERIF nem substituir automaticamente essa medição por uma biblioteca desktop.

## Leitor IL2CPP

StreamUtil.ReadFloat14446 (0x1c0c764) chama StreamUtil.Read14430 (0x1c0be50) com comprimento4 e encaminha o array a EncodeUtil.ByteAryToFloat13770 (0x1bf3628), passando true. Este chama ConvertEndian13777 (0x1bf2f90) com4 e depois BitConverter.ToSingle1055 (0x1dddbbc), índice0.

ConvertEndian troca os bytes simétricos do array de quatro bytes. ToSingle chama o leitor inteiro em0x1dddaa8 e usa FMOV para reinterpretar os bits como float, sem conversão numérica inteira→real. O corpo desse último leitor inteiro não foi exportado neste ciclo; a cadeia observada documenta a inversão explícita e a reinterpretação, sem depender apenas do nome ReadFloat.

O delegate lê dois valores e constrói SizeF, conforme97. Não foi acrescentado um codec ao port: o protocolo JNI não é necessário por si só para um renderer nativo, e a seleção efetiva dessa rota nas falas ainda está aberta.

## Evidência e validação

- APK SHA-256: `d58943e6ed4137975a554f1e210853f4a7b41573a38dbb462f4f5e159bf6eead`.
- Busca REA: `ev_3b1dcd9e9892f06aceff4a162bc5303e0e608567795f345646550c0afc7d53dc`; envelope completo local em `build/research/font-java-classes.json`. A busca identifica nomes, não corpos.
- Corpo DEX exportado pela API JADX instalada com `tools/ExportSmali.java`; local `build/research/font-java-smali/kairo.android.plugin.Utility.smali`, método iniciado na linha1870. SHA do APK registrado junto da exportação. Nenhum código decompilado versionado.
- Ghidra: `build/research/font-java-binding/`, `font-float-decoder/` e `font-float-bits/`, com TSVs e logs correspondentes. Endereços nos arquivos assembly incluem base adicional0x100000; os endereços deste relatório são VAs do ELF.
- Cobertura sintática integral das funções exportadas; não é cobertura de execução. Nenhuma alteração C++ ou teste de produto repetido; último build registrado permanece18/18.

Confiança **confirmed** para ligação, chamadas DEX e sequência do leitor; **unknown** para seleção efetiva de useFontTexture_ e métricas de uma partida. Próximo: rastrear a fonte criada no boot e os escritores do flag antes de escolher o backend do layout das falas. Ainda há investigação estática útil.
