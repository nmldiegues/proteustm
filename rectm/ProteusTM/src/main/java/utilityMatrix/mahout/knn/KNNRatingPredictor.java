package utilityMatrix.mahout.knn;

import org.apache.mahout.cf.taste.common.TasteException;
import org.apache.mahout.cf.taste.impl.neighborhood.NearestNUserNeighborhood;
import org.apache.mahout.cf.taste.impl.recommender.GenericItemBasedRecommender;
import org.apache.mahout.cf.taste.impl.recommender.GenericUserBasedRecommender;
import org.apache.mahout.cf.taste.impl.similarity.EuclideanDistanceSimilarity;
import org.apache.mahout.cf.taste.impl.similarity.LogLikelihoodSimilarity;
import org.apache.mahout.cf.taste.impl.similarity.PearsonCorrelationSimilarity;
import org.apache.mahout.cf.taste.impl.similarity.UncenteredCosineSimilarity;
import org.apache.mahout.cf.taste.model.DataModel;
import org.apache.mahout.cf.taste.neighborhood.UserNeighborhood;
import org.apache.mahout.cf.taste.recommender.Recommender;
import org.apache.mahout.cf.taste.similarity.ItemSimilarity;
import org.apache.mahout.cf.taste.similarity.UserSimilarity;
import utilityMatrix.mahout.MahoutRatingPredictor;
import utilityMatrix.mahout.knn.similWrappers.ItemAwareSimilarityWrapper;
import utilityMatrix.recoEval.profiles.ProfilesHolder;
import utilityMatrix.recoEval.profiles.UserProfile;
import utilityMatrix.recoEval.tools.ExtendedParameters;

/**
 * @author Diego Didona
 * @email didona@gsd.inesc-id.pt
 * @since 03/09/14
 */
public class KNNRatingPredictor extends MahoutRatingPredictor<KNNConfig> {

   private DataModel dataModelPtr;

   public KNNRatingPredictor(ProfilesHolder<UserProfile> training, ProfilesHolder<UserProfile> testing, String output, ExtendedParameters d, KNNConfig config) {
      super(training, testing, output, d, config);
   }

   /**
    * Build a recommender starting from a DataModel. The wrapping mode will be disabled at buildRecommender() time,
    * since we do not know the target item, but not at buildRecommenderOnTheFly() mode, as we will know the target item
    * we want a rating for
    *
    * @param dataModel
    * @param useWrapper
    * @param targetItem
    * @return
    */
   private Recommender buildRecommenderOnDataModel(DataModel dataModel, boolean useWrapper, int targetItem) {
      final boolean isUserSimilarity = this.config.isUserSimilarity();
      if (!isUserSimilarity) {
         final ItemSimilarity itemSimilarity = buildItemSimilarity(dataModel);
         return buildItemKNNRecommender(dataModel, itemSimilarity);
      }
      try {
         final UserSimilarity userSimilarity = buildUserSimilarity(dataModel, useWrapper, targetItem);
         return buildUserKNNRecommender(dataModel, userSimilarity);
      } catch (TasteException e) {
         throw new RuntimeException(e);
      }
   }

   @Override
   //TODO: Beware of what happens when you do the retrain
   protected Recommender buildRecommender() {
      this.dataModelPtr = this.mahoutAux.fromProfilesHolderToGenericDataModel();
      return buildRecommenderOnDataModel(this.dataModelPtr, false, -1);
   }

   private ItemSimilarity buildItemSimilarity(DataModel dataModel) {
      final String similarity = this.config.getSimilarity();
      try {
         if (similarity.equals("PEARSON")) {
            return new PearsonCorrelationSimilarity(dataModel, this.config.getWeighting());
         }
         if (similarity.equals("COSINE")) {
            return new UncenteredCosineSimilarity(dataModel, this.config.getWeighting());
         }
         if (similarity.equals("LOG")) {
            return new LogLikelihoodSimilarity(dataModel);
         }
         if (similarity.equals("EUCLIDEAN")) {
            return new EuclideanDistanceSimilarity(dataModel, this.config.getWeighting());
         }
         throw new IllegalArgumentException("Similarity " + similarity + " not recognized");
      } catch (TasteException e) {
         throw new RuntimeException(e);
      }
   }

   private UserSimilarity buildUserSimilarity(DataModel dataModel) {
      final String similarity = this.config.getSimilarity();
      try {
         if (similarity.equals("PEARSON")) {
            return new PearsonCorrelationSimilarity(dataModel, this.config.getWeighting());
         }
         if (similarity.equals("COSINE")) {
            return new UncenteredCosineSimilarity(dataModel, this.config.getWeighting());
         }
         if (similarity.equals("LOG")) {
            return new LogLikelihoodSimilarity(dataModel);
         }
         if (similarity.equals("EUCLIDEAN")) {
            return new EuclideanDistanceSimilarity(dataModel, this.config.getWeighting());
         }
         throw new IllegalArgumentException("Similarity " + similarity + " not recognized");
      } catch (TasteException e) {
         throw new RuntimeException(e);
      }
   }

   private UserSimilarity buildUserSimilarity(DataModel dataModel, boolean useWrapper, int targetItem) {
      final UserSimilarity userSimilarity = buildUserSimilarity(dataModel);
      if (!useWrapper) {
         return userSimilarity;
      }
      return new ItemAwareSimilarityWrapper(userSimilarity, targetItem, dataModel);
   }

   /**
    * @return true if we are now using item similarity or if we are in user-similairty with no per-item neighborhood
    */
   protected boolean isRebuildOnTheFly() {
      if (config.getSimilarity().equals("ITEM_SIMILARITY") || !config.isItemBasedNeighborhood())
         return false;
      return true;
   }

   @Override
   protected Recommender buildRecommenderOnTheFly(int user, int item) {
      final boolean isItemNeighborhood = config.isItemBasedNeighborhood();
      return buildRecommenderOnDataModel(this.dataModelPtr, isItemNeighborhood, item);
   }

   private Recommender buildItemKNNRecommender(DataModel dataModel, ItemSimilarity itemSimilarity) {
      return new GenericItemBasedRecommender(dataModel, itemSimilarity);
   }

   private Recommender buildUserKNNRecommender(DataModel dataModel, UserSimilarity userSimilarity) throws TasteException {
      final UserNeighborhood neighborhood = new NearestNUserNeighborhood(this.config.getNumNeighbors(), userSimilarity, dataModel);
      GenericUserBasedRecommender genericUserBasedRecommender = new GenericUserBasedRecommender(dataModel, neighborhood, userSimilarity);
      return genericUserBasedRecommender;
   }
}
