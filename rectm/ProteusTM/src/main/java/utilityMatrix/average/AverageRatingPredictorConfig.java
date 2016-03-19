package utilityMatrix.average;

import utilityMatrix.RecommenderConfig;

/**
 * @author Diego Didona
 * @email didona@gsd.inesc-id.pt
 * @since 19/10/14
 */
public class AverageRatingPredictorConfig extends RecommenderConfig {

   private avgBy averageMethod = avgBy.COLUMN;

   public avgBy getAverageMethod() {
      return averageMethod;
   }

   public boolean isByColumn() {
      return this.averageMethod.equals(avgBy.COLUMN);
   }

   public boolean isByRow() {
      return this.averageMethod.equals(avgBy.ROW);
   }

   public void setAverageMethod(String averageMethod) {
      this.averageMethod = avgBy.valueOf(averageMethod);
   }

   public enum avgBy {
      ROW, COLUMN
   }

   @Override
   public String toString() {
      return super.toString() + "AverageRatingPredictorConfig{" +
            "averageMethod=" + averageMethod +
            '}';
   }
}
