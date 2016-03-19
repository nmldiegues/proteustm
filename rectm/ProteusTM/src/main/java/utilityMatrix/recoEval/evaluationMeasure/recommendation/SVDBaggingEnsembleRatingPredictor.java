package utilityMatrix.recoEval.evaluationMeasure.recommendation;

import evaluation.runtimeSimul.configurations.SVDEnsembleRatingPredictorConfig;
import gnu.trove.map.TIntObjectMap;
import gnu.trove.map.hash.TIntObjectHashMap;
import utilityMatrix.mahout.svd.SVDConfig;
import utilityMatrix.mahout.svd.SVDRatingPredictor;
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
 * @since 21/11/14
 */
public class SVDBaggingEnsembleRatingPredictor extends EnsembleExtendedRatingPredictor<SVDEnsembleRatingPredictorConfig> {


   public SVDBaggingEnsembleRatingPredictor(ProfilesHolder<UserProfile> training, ProfilesHolder<UserProfile> testing, String output, ExtendedParameters d, SVDEnsembleRatingPredictorConfig con) {
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

   protected List<ExtendedRatingPredictor> buildRatingPredictors(SVDEnsembleRatingPredictorConfig config, int targetUser, int targetItem) {
      final int num = config.getNumPredictors();
      list.clear();
      for (int i = 1; i <= num; i++) {
         SVDConfig baseConfig = new DXmlParser<SVDConfig>().parse(config.getBaseConfig());
         //The base learners MUST be able to return a null value
         //The ensembler has the power of averaging if it wills
         baseConfig.setAllowNullRatings(true);
         baseConfig.setNumItemsPerUser(this.config.getNumItemsPerUser());
         ProfilesHolder<UserProfile> trainForI = userProfilesFor(i, targetUser, targetItem);
         list.add(new SVDRatingPredictor(trainForI, testing_, this.output_, this.extendedParameters, baseConfig));
      }
      return list;
   }


   private ProfilesHolder<UserProfile> userProfilesFor(int i, int targetUser, int targetItem) {
      Random random = new Random(i * extendedParameters.getSeed());
      TIntObjectMap<UserProfile> profiles = new TIntObjectHashMap<>();
      double cutoff = config.getBaggingPercentage();
      for (int u : training_.getUserIds()) {
         if (random.nextDouble() <= cutoff || u == targetUser)
            profiles.put(u, training_.getProfile(u));
      }
      BasicUserProfileHolder<UserProfile> ret = new BasicUserProfileHolder<>(profiles);

      return ret;
   }

}
