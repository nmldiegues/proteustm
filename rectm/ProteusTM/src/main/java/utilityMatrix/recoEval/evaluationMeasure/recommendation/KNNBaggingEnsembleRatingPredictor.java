package utilityMatrix.recoEval.evaluationMeasure.recommendation;

import evaluation.runtimeSimul.configurations.KNNEnsembleRatingPredictorConfig;
import gnu.trove.map.TIntObjectMap;
import gnu.trove.map.hash.TIntObjectHashMap;
import utilityMatrix.mahout.knn.KNNConfig;
import utilityMatrix.mahout.knn.KNNRatingPredictor;
import utilityMatrix.recoEval.profiles.BasicUserProfileHolder;
import utilityMatrix.recoEval.profiles.ProfilesHolder;
import utilityMatrix.recoEval.profiles.UserProfile;
import utilityMatrix.recoEval.tools.ExtendedParameters;
import xml.DXmlParser;

import java.util.List;
import java.util.Random;

/**
 * @author Diego Didona
 * @email didona@gsd.inesc-id.pt
 * @since 14/11/14
 */
public class KNNBaggingEnsembleRatingPredictor extends EnsembleExtendedRatingPredictor<KNNEnsembleRatingPredictorConfig> {

   /*The bagged learners all share pointers to the same UserProfiles: there is only one profile
   * for each row*/


   public KNNBaggingEnsembleRatingPredictor(ProfilesHolder<UserProfile> training, ProfilesHolder<UserProfile> testing, String output, ExtendedParameters d, KNNEnsembleRatingPredictorConfig con) {
      super(training, testing, output, d, con);
   }


   @Override
   protected boolean isRebuildOnTheFly() {
      return false; //Now we are using isRebuildOnUserBase!
   }

   @Override
   protected boolean rebuildOnUserBase(int user) {
      return true;
   }

   @Override
   protected void rebuild(int user) {
      this.extendedRatingPredictorList = buildRatingPredictors(config, user, NO_ITEM);
   }

   protected List<ExtendedRatingPredictor> buildRatingPredictors(KNNEnsembleRatingPredictorConfig config, int targetUser, int targetItem) {
      final int num = config.getNumPredictors();
      list.clear();
      for (int i = 1; i <= num; i++) {
         KNNConfig baseConfig = new DXmlParser<KNNConfig>().parse(config.getBaseConfig());
         //The base learners MUST be able to return a null value
         //The top-level ensemble learner, then deals with it
         baseConfig.setAllowNullRatings(true);
         baseConfig.setNumItemsPerUser(this.config.getNumItemsPerUser());
         ProfilesHolder<UserProfile> trainForI = userProfilesFor(i, targetUser, targetItem);

         list.add(new KNNRatingPredictor(trainForI, testing_, this.output_, this.extendedParameters, baseConfig));
      }
      return list;
   }


   private ProfilesHolder<UserProfile> userProfilesFor(int i, int targetUser, int targetItem) {
      final Random random = new Random(i * extendedParameters.getSeed());
      final TIntObjectMap<UserProfile> profiles = new TIntObjectHashMap<>();
      final double cutoff = config.getBaggingPercentage();
      for (int u : training_.getUserIds()) {
         if (random.nextDouble() <= cutoff || u == targetUser)
            profiles.put(u, training_.getProfile(u));
      }
      return new BasicUserProfileHolder<>(profiles);
   }

   @Override
   public String toString() {
      return super.toString() + " KNNBaggingEnsembleRatingPredictor{}";
   }
}
