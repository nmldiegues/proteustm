package utilityMatrix.mahout.random;

import org.apache.mahout.cf.taste.common.TasteException;
import org.apache.mahout.cf.taste.impl.recommender.RandomRecommender;
import org.apache.mahout.cf.taste.model.DataModel;
import org.apache.mahout.cf.taste.recommender.Recommender;
import utilityMatrix.mahout.MahoutRatingPredictor;
import utilityMatrix.recoEval.profiles.ProfilesHolder;
import utilityMatrix.recoEval.profiles.UserProfile;
import utilityMatrix.recoEval.tools.ExtendedParameters;

/**
 * @author Diego Didona
 * @email didona@gsd.inesc-id.pt
 * @since 05/09/14 NB: I have implemented this as it seems that the random recommender already encoded just randomizes
 * the weights for neighbors. I want something that gives me random recommendation
 */
public class RandomRatingPredictor extends MahoutRatingPredictor<RandomConfig> {

   public RandomRatingPredictor(ProfilesHolder<UserProfile> training, ProfilesHolder<UserProfile> testing, String output, ExtendedParameters d, RandomConfig config) {
      super(training, testing, output, d, config);
   }

   @Override
   protected Recommender buildRecommender() {
      DataModel dataModel = this.mahoutAux.fromProfilesHolderToGenericDataModel();
      try {
         //This will give random estimates, so, overall, the recommended configuration will be random
         return new RandomRecommender(dataModel);
      } catch (TasteException e) {
         throw new RuntimeException(e);
      }
   }

   @Override
   protected Recommender buildRecommenderOnTheFly(int user, int item) {
      throw new IllegalStateException("For now this is only for user based knn or cnn");
   }
}
