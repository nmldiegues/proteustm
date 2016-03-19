package utilityMatrix.average;

import gnu.trove.iterator.TIntIterator;
import utilityMatrix.recoEval.evaluationMeasure.recommendation.ExtendedRatingPredictor;
import utilityMatrix.recoEval.profiles.ProfilesHolder;
import utilityMatrix.recoEval.profiles.UserProfile;
import utilityMatrix.recoEval.tools.ExtendedParameters;

/**
 * @author Diego Didona
 * @email didona@gsd.inesc-id.pt
 * @since 19/10/14
 */
public class AverageRatingPredictor extends ExtendedRatingPredictor<AverageRatingPredictorConfig> {

   public AverageRatingPredictor(ProfilesHolder<UserProfile> training, ProfilesHolder<UserProfile> testing, String output, ExtendedParameters d, AverageRatingPredictorConfig con) {
      super(training, testing, output, d, con);
   }

   @Override
   protected boolean rebuildOnUserBase(int user) {
      return false;  // TODO: Customise this generated block
   }

   @Override
   protected void rebuild(int user) {
      // TODO: Customise this generated block
   }

   @Override
   /**
    * Note that the diffNormalization is useless here, as we are retuning average values
    * //TODO should I check mathematically this? I think I have already done this
    */
   protected RatingPrediction _predictRatingForUser(int user, int item) {
      RatingPrediction toRet;
      if (config.isByColumn()) {
         toRet = predictRatingForUserByColumn(user, item);
      } else if (config.isByRow()) {
         toRet = predictRatingForUserByRow(user, item);
      } else {
         throw new IllegalArgumentException("AvgRatingConfig is neither by row nor by column " + config);
      }
      return toRet;
   }


   @Override
   protected void _retrain(boolean resetStats) {
      //NOP: everything here is computed on the fly
   }

   /**
    * Predict the rating of an item using the average of the ratings of the user
    *
    * @param user
    * @param item
    * @return
    */
   private RatingPrediction predictRatingForUserByRow(int user, int item) {
      UserProfile userProfile = training_.getProfile(user);
      TIntIterator it = userProfile.getItems();
      double sum = 0, count = 0;
      while (it.hasNext()) {
         if (!isActualRating(item)) {
            continue;
         }
         int itemId = it.next();
         sum += userProfile.getRating(itemId);
         count++;
      }
      return new RatingPrediction(item, sum / count);
   }

   /**
    * Predict the rating of an item using the average of all the ratings available for that item
    *
    * @param user
    * @param item
    * @return
    */
   private RatingPrediction predictRatingForUserByColumn(int user, int item) {
      int[] trainingIds = training_.getUserIds();
      if (!isActualRating(item)) {
         throw new IllegalArgumentException("Why are you asking the average of a non-rating by column?");
      }
      double sum = 0, count = 0;
      for (int userId : trainingIds) {
         UserProfile userProfile = training_.getProfile(userId);
         if (userProfile.contains(item)) {
            sum += userProfile.getRating(item);
            count++;
         }
      }
      return new RatingPrediction(item, sum / count);
   }

}
