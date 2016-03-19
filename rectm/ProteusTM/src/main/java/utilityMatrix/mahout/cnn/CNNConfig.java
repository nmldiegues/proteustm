package utilityMatrix.mahout.cnn;

import org.apache.mahout.cf.taste.common.Weighting;
import utilityMatrix.mahout.MahoutRecommenderConfig;

/**
 * @author Diego Didona
 * @email didona@gsd.inesc-id.pt
 * @since 03/09/14
 */
public class CNNConfig extends MahoutRecommenderConfig {

   private double cutoff;
   private String similarity;
   private Weighting weighting;


   public double getCutoff() {
      return cutoff;
   }

   public void setCutoff(double cutoff) {
      this.cutoff = cutoff;
   }

   public String getSimilarity() {
      return similarity;
   }

   public void setSimilarity(String similarity) {
      this.similarity = similarity;
   }

   public Weighting getWeighting() {
      return weighting;
   }

   public void setWeighting(String weighting) {
      this.weighting = Weighting.valueOf(weighting);
   }

   @Override
   public String toString() {
      return "CNNConfig{" +
            "cutoff=" + cutoff +
            ", similarity='" + similarity + '\'' +
            ", weighting=" + weighting +
            '}';
   }
}
