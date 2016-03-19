package utilityMatrix.mahout.random;

import utilityMatrix.mahout.MahoutRecommenderConfig;

/**
 * @author Diego Didona
 * @email didona@gsd.inesc-id.pt
 * @since 05/09/14
 */
public final class RandomConfig extends MahoutRecommenderConfig {

   private long seed;

   public long getSeed() {
      return seed;
   }

   public void setSeed(long seed) {
      this.seed = seed;
   }

   @Override
   public String toString() {
      return "RandomConfig{" +
            "seed=" + seed +
            '}';
   }
}
