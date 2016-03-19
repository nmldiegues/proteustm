package runtime;

/**
 * @author Diego Didona
 * @email didona@gsd.inesc-id.pt
 * @since 18/02/15
 */
public interface IntegerToRuntimeConfig {

   String LINEAR = "LINEAR";
   String HALF = "HALF";
   String GIVEUP = "GIVEUP";

   public String fromIntToRuntimeConfig(String rawCfg);

}
