package evaluation.configuration.integer;

import evaluation.runtimeSimul.configurations.ConfigurationSelectorConfig;
import evaluation.workload.Workload;
import gnu.trove.iterator.TIntIterator;
import utilityMatrix.recoEval.evaluationMeasure.recommendation.ExtendedRatingPredictor;
import utilityMatrix.recoEval.evaluationMeasure.recommendation.PerUserRecommendationResult;
import utilityMatrix.recoEval.profiles.UserProfile;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Random;

/**
 * @author Diego Didona
 * @email didona@gsd.inesc-id.pt
 * @since 19/09/14
 */
public class GreedyIntegerConfigurationSelector extends AbstractConfigurationSelector<IntegerConfiguration> {
   private final Random random;

   public GreedyIntegerConfigurationSelector(List<IntegerConfiguration> steadyStateList, ConfigurationSelectorConfig selectorConfiguration, ExtendedRatingPredictor ratingPredictor, Workload testingWorkload) {
      super(steadyStateList, selectorConfiguration, ratingPredictor, testingWorkload);
      this.random = new Random(selectorConfiguration.getRandomSeed());
   }

   public GreedyIntegerConfigurationSelector(List<IntegerConfiguration> bootstrapList, List<IntegerConfiguration> steadyStateList, ConfigurationSelectorConfig selectorConfiguration, ExtendedRatingPredictor ratingPredictor, Workload workload) {
      super(bootstrapList, steadyStateList, selectorConfiguration, ratingPredictor, workload);
      this.random = new Random(selectorConfiguration.getRandomSeed());
   }

   @Override
   protected IntegerConfiguration _next() {
      //Query for all unknown configurations and return the highest ranked
      final PerUserRecommendationResult recommendationResult = ratingPredictor.computeRecommendations(currentTestWorkload.getId());
      /*NOTE: best config in recommendationResult may be one of the already seen, and I do not want that, so I loop*/
      //TODO: have the result to return the actual ranking of configurations with KPI

      UserProfile predProfile = recommendationResult.getPredictedProfile();
      //The predicted profile should only have the unknown configs, but to be sure...
      TIntIterator it = currentTestWorkload.getProfile().getItems();
      boolean higherBetter = selectorConfiguration.isHigherBetter();
      double bestRank = higherBetter ? Double.MIN_VALUE : Double.MAX_VALUE;
      List<Integer> candidates = new ArrayList<>();
      while (it.hasNext()) {
         int id = it.next();
         double pred = predProfile.getRating(id);
         //Update the max if present
         if (isStrictlyBetterThan(pred, bestRank)) {
            bestRank = pred;
            candidates.clear();
            candidates.add(id);
         }
         //If we have a tie, we add it to the list
         else if (pred == bestRank) {
            candidates.add(id);
         }
      }
      if (candidates.size() > 1) {
         Collections.shuffle(candidates, this.random);
      }
      /*
      This should never happen :(
       */
      if (candidates.size() == 0) {
         if (info) {
            log.info("Could not perform greedy step. Going random");
            int rnd = this.random.nextInt(currentList.size());
            return currentList.get(rnd);
         }
      }
      return new IntegerConfiguration(candidates.get(0));

   }

   private boolean isStrictlyBetterThan(double a, double b) {
      boolean higherBetter = selectorConfiguration.isHigherBetter();
      if (higherBetter) {
         return a > b;
      }
      return a < b;
   }

   @Override
   protected boolean _terminateExploration() {
      return false; //Delegate to previous predicate
   }

}
