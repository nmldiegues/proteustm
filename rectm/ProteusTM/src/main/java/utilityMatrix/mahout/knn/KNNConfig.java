package utilityMatrix.mahout.knn;

import org.apache.mahout.cf.taste.common.Weighting;
import utilityMatrix.mahout.MahoutRecommenderConfig;

/**
 * @author Diego Didona
 * @email didona@gsd.inesc-id.pt
 * @since 03/09/14
 */
public class KNNConfig extends MahoutRecommenderConfig {

   private int numNeighbors;
   private String similarity;
   private Weighting weighting;
   private userItemSimilarityEnum userItemSimilarity = userItemSimilarityEnum.USER_SIMILARITY;
   private boolean itemBasedNeighborhood = false;

   public boolean isItemBasedNeighborhood() {
      return itemBasedNeighborhood;
   }

   public void setItemBasedNeighborhood(String itemBasedNeighborhood) {
      this.itemBasedNeighborhood = Boolean.valueOf(itemBasedNeighborhood);
   }

   public void setItemBasedNeighborhood(boolean itemBasedNeighborhood) {
      this.itemBasedNeighborhood = itemBasedNeighborhood;
   }

   public void setUserItemSimilarity(String userItemSimilarity) {
      this.userItemSimilarity = userItemSimilarityEnum.valueOf(userItemSimilarity);
   }

   public userItemSimilarityEnum getUserItemSimilarity() {
      return userItemSimilarity;
   }

   public Weighting getWeighting() {
      return weighting;
   }

   public void setWeighting(String weighting) {
      this.weighting = Weighting.valueOf(weighting);
   }

   public String getSimilarity() {
      return similarity;
   }

   public void setSimilarity(String similarity) {
      this.similarity = similarity;
   }

   public int getNumNeighbors() {
      return numNeighbors;
   }

   public void setNumNeighbors(int numNeighbors) {
      this.numNeighbors = numNeighbors;
   }

   public boolean isUserSimilarity() {
      return this.userItemSimilarity.equals(userItemSimilarityEnum.USER_SIMILARITY);
   }

   @Override
   public String toString() {
      return "KNNConfig{" +
            "numNeighbors=" + numNeighbors +
            ", similarity='" + similarity + '\'' +
            ", weighting=" + weighting +
            ", userItemSimilarity=" + userItemSimilarity +
            ", itemBasedNeighborhood=" + itemBasedNeighborhood +
            '}';
   }

   public enum userItemSimilarityEnum {
      USER_SIMILARITY, ITEM_SIMILARITY
   }
}
