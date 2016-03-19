package utilityMatrix.recoEval.evaluationMeasure.recommendation;

import utilityMatrix.recoEval.profiles.UserProfile;

public class PerUserRecommendationResult {
   private UserProfile predictedProfile;
   private int predOptimalConfigId;

   public PerUserRecommendationResult(UserProfile predictedProfile, int predOptimalConfigId) {
      this.predictedProfile = predictedProfile;
      this.predOptimalConfigId = predOptimalConfigId;
   }


   public UserProfile getPredictedProfile() {
      return predictedProfile;
   }

   public int getPredOptimalConfigId() {
      return predOptimalConfigId;
   }


   @Override
   public String toString() {
      return "" +
            "\npredictedProfile=" + predictedProfile +
            "\npredOptimalConfigId=" + predOptimalConfigId;
   }
}