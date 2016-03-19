package utilityMatrix.mahout.cnn;

import org.apache.mahout.cf.taste.common.TasteException;
import org.apache.mahout.cf.taste.impl.neighborhood.ThresholdUserNeighborhood;
import org.apache.mahout.cf.taste.impl.recommender.GenericUserBasedRecommender;
import org.apache.mahout.cf.taste.impl.similarity.PearsonCorrelationSimilarity;
import org.apache.mahout.cf.taste.impl.similarity.UncenteredCosineSimilarity;
import org.apache.mahout.cf.taste.model.DataModel;
import org.apache.mahout.cf.taste.neighborhood.UserNeighborhood;
import org.apache.mahout.cf.taste.recommender.Recommender;
import org.apache.mahout.cf.taste.similarity.UserSimilarity;
import utilityMatrix.mahout.MahoutRatingPredictor;
import utilityMatrix.recoEval.profiles.ProfilesHolder;
import utilityMatrix.recoEval.profiles.UserProfile;
import utilityMatrix.recoEval.tools.ExtendedParameters;

/**
 * @author Diego Didona
 * @email didona@gsd.inesc-id.pt
 * @since 03/09/14
 */
public class CNNRatingPredictor extends MahoutRatingPredictor<CNNConfig> {


   public CNNRatingPredictor(ProfilesHolder<UserProfile> training, ProfilesHolder<UserProfile> testing, String output, ExtendedParameters d, CNNConfig config) {
      super(training, testing, output, d, config);
   }

   @Override
   protected Recommender buildRecommender() {
      final int nbItems = this.config.getNumItemsPerUser();
      DataModel dataModel = this.mahoutAux.fromProfilesHolderToGenericDataModel();
      // MahoutAux.fromProfilesHolderToFileDataModel(training_, nbItems, "UtilityMatrix/conf/temp.data");
      UserSimilarity userSimilarity = buildUserSimilarity(dataModel);
      UserNeighborhood neighborhood = new ThresholdUserNeighborhood(this.config.getCutoff(), userSimilarity, dataModel);
      return new GenericUserBasedRecommender(dataModel, neighborhood, userSimilarity);
   }

   private UserSimilarity buildUserSimilarity(DataModel dataModel) {
      final String similarity = this.config.getSimilarity();
      try {
         if (similarity.equals("PEARSON"))
            return new PearsonCorrelationSimilarity(dataModel, this.config.getWeighting());
         if (similarity.equals("COSINE"))
            return new UncenteredCosineSimilarity(dataModel, this.config.getWeighting());
         //new EuclideanDistanceSimilarity()
         throw new IllegalArgumentException("Similarity " + similarity + " not recognized");
      } catch (TasteException e) {
         throw new RuntimeException(e);
      }
   }

   @Override
     protected Recommender buildRecommenderOnTheFly(int user, int item) {
        throw new IllegalStateException("For now this is only for user based knn or cnn");
     }
}
