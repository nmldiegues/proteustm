package runtime;


import java.io.IOException;

/**
 * @author Diego Didona
 * @email didona@gsd.inesc-id.pt
 * @since 18/02/15
 */
public abstract class IntegerToRuntimeConfigFactory {

   public static String fromIntToStringCfg(String basefile, String parserClass, int cfgId) {
      try {
         String rawString = new DoubleOutputParser(basefile, ",").headerIfFor(cfgId);
         Class clazz = Class.forName(parserClass);
         IntegerToRuntimeConfig integerToRuntimeConfig = (IntegerToRuntimeConfig) clazz.newInstance();
         return integerToRuntimeConfig.fromIntToRuntimeConfig(rawString);
      } catch (Exception e) {
         throw new RuntimeException(e);
      }
   }

}
