package utilityMatrix.mahout;

import utilityMatrix.RecommenderConfig;

/**
 * @author Diego Didona
 * @email didona@gsd.inesc-id.pt
 * @since 03/09/14
 */
public abstract class MahoutRecommenderConfig extends RecommenderConfig {
   private int numItemsPerUser;

   public int getNumItemsPerUser() {
      return numItemsPerUser;
   }

   public void setNumItemsPerUser(int numItemsPerUser) {
      this.numItemsPerUser = numItemsPerUser;
   }

   @Override
   public String toString() {
      return super.toString() + "MahoutRecommenderConfig{" +
            "numItemsPerUser=" + numItemsPerUser +
            '}';
   }
}
