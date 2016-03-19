package evaluation.runtimeSimul.configurations;

/**
 * @author Diego Didona
 * @email didona@gsd.inesc-id.pt
 * @since 15/09/14
 */
public class SVDEnsembleRatingPredictorConfig extends EnsembleRatingPredictorConfig {

   private ensemblingMode mode = ensemblingMode.NOISE;
   private String baseConfig = null;
   private final double baggingPercentage = .66;

   public void setBaseConfig(String baseConfig) {
      this.baseConfig = baseConfig;
   }

   public String getBaseConfig() {
      return baseConfig;
   }

   public ensemblingMode getMode() {
      return mode;
   }

   public boolean ensembleViaNoise() {
      return this.mode.equals(ensemblingMode.NOISE);
   }

   public void setEnsemblingMode(String mode) {
      this.mode = ensemblingMode.valueOf(mode);
   }

   public double getBaggingPercentage() {
      return baggingPercentage;
   }

   private enum ensemblingMode {
      LATENT_FEATURES, NOISE, BAGGING
   }
}
