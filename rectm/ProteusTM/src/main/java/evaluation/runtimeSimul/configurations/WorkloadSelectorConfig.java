package evaluation.runtimeSimul.configurations;

/**
 * @author Diego Didona
 * @email didona@gsd.inesc-id.pt
 * @since 15/09/14
 */
public class WorkloadSelectorConfig {

   private wsc mode = wsc.RANDOM;
   private long randomSeed;

   public long getRandomSeed() {
      return randomSeed;
   }

   public void setRandomSeed(long randomSeed) {
      this.randomSeed = randomSeed;
   }

   public boolean isSequential() {
      return this.mode == wsc.SEQUENTIAL;
   }

   public boolean isRandom() {
      return this.mode == wsc.RANDOM;
   }

   public void setMode(String mode) {
      this.mode = wsc.valueOf(mode);
   }

   private enum wsc {
      SEQUENTIAL, RANDOM
   }

   @Override
   public String toString() {
      return "WorkloadSelectorConfig{" +
            "mode=" + mode +
            ", randomSeed=" + randomSeed +
            '}';
   }
}
