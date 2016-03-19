package evaluation.runtimeSimul.configurations;

/**
 * @author Diego Didona
 * @email didona@gsd.inesc-id.pt
 * @since 14/11/14
 */
public class KNNEnsembleRatingPredictorConfig extends EnsembleRatingPredictorConfig {

   private String baseConfig = null;
   private ensemblingMode mode = ensemblingMode.BAGGING;
   private double baggingPercentage = .632;     //Drawing with replacement N times tends to be equal to drawing without replacement about N*0.632

   public double getBaggingPercentage() {
      return baggingPercentage;
   }

   public void setBaggingPercentage(double baggingPercentage) {
      this.baggingPercentage = baggingPercentage;
   }

   public String getBaseConfig() {
      return baseConfig;
   }

   public void setMode(ensemblingMode mode) {
      this.mode = mode;
   }

   public void setMode(String mode) {
      this.mode = ensemblingMode.valueOf(mode);
   }

   public void setBaseConfig(String baseConfig) {
      this.baseConfig = baseConfig;
   }

   public ensemblingMode getMode() {
      return mode;
   }

   private enum ensemblingMode {
      NEIGHBORS, BAGGING
   }

   @Override
   public String toString() {
      return super.toString() + " KNNEnsembleRatingPredictorConfig{" +
            "baseConfig='" + baseConfig + '\'' +
            ", mode=" + mode +
            ", baggingPercentage=" + baggingPercentage +
            '}';
   }
}
