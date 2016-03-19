package evaluation.runtimeSimul.configurations;

import utilityMatrix.RecommenderConfig;

/**
 * @author Diego Didona
 * @email didona@gsd.inesc-id.pt
 * @since 15/09/14
 */
public abstract class EnsembleRatingPredictorConfig extends RecommenderConfig {
   private int numItemsPerUser;
   private int numThreads = 1;
   private int pollingTime = 10; //msec
   private int numPredictors;

   public int getNumPredictors() {
      return numPredictors;
   }

   public void setNumPredictors(int numPredictors) {
      this.numPredictors = numPredictors;
   }

   public int getPollingTime() {
      return pollingTime;
   }

   public void setPollingTime(int pollingTime) {
      this.pollingTime = pollingTime;
   }

   public int getNumThreads() {
      return numThreads;
   }

   public void setNumThreads(int numThreads) {
      this.numThreads = numThreads;
   }

   public void setNumItemsPerUser(int numItemsPerUser) {
      this.numItemsPerUser = numItemsPerUser;
   }

   public int getNumItemsPerUser() {
      return numItemsPerUser;
   }

   @Override
   public String toString() {
      return super.toString() + " " + this.getClass().toString() + "{" +
            "numItemsPerUser=" + numItemsPerUser +
            ", numThreads=" + numThreads +
            ", pollingTime=" + pollingTime +
            ", numPredictors=" + numPredictors +
            '}';
   }
}
