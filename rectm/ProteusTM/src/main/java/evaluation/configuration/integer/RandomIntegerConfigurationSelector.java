package evaluation.configuration.integer;

import evaluation.runtimeSimul.configurations.ConfigurationSelectorConfig;
import evaluation.workload.Workload;
import utilityMatrix.recoEval.evaluationMeasure.recommendation.ExtendedRatingPredictor;

import java.util.Collections;
import java.util.List;
import java.util.Random;

/**
 * @author Diego Didona
 * @email didona@gsd.inesc-id.pt
 * @since 14/09/14
 */
//I have to choose: either this extends randomSelector or thr Configuration one. I opt for the second

public class RandomIntegerConfigurationSelector extends AbstractConfigurationSelector<IntegerConfiguration> {

   private final Random random;

   public RandomIntegerConfigurationSelector(List<IntegerConfiguration> steadyStateList, ConfigurationSelectorConfig selectorConfiguration, ExtendedRatingPredictor ratingPredictor, Workload workload, Random random) {
      super(steadyStateList, selectorConfiguration, ratingPredictor, workload);
      this.random = random;
      Collections.shuffle(this.steadyStateList, this.random);
   }

   public RandomIntegerConfigurationSelector(List<IntegerConfiguration> bootstrapList, List<IntegerConfiguration> steadyStateList, ConfigurationSelectorConfig selectorConfiguration, ExtendedRatingPredictor ratingPredictor, Workload workload, Random random) {
      super(bootstrapList, steadyStateList, selectorConfiguration, ratingPredictor, workload);
      this.random = random;
      Collections.shuffle(this.steadyStateList, this.random);
   }

   public RandomIntegerConfigurationSelector(List<IntegerConfiguration> steadyStateList, ConfigurationSelectorConfig selectorConfiguration, ExtendedRatingPredictor ratingPredictor, Workload workload) {
      super(steadyStateList, selectorConfiguration, ratingPredictor, workload);
      random = new Random(selectorConfiguration.getRandomSeed());
      Collections.shuffle(this.steadyStateList, this.random);
   }

   public RandomIntegerConfigurationSelector(List<IntegerConfiguration> bootstrapList, List<IntegerConfiguration> steadyStateList, ConfigurationSelectorConfig selectorConfiguration, ExtendedRatingPredictor ratingPredictor, Workload workload) {
      super(bootstrapList, steadyStateList, selectorConfiguration, ratingPredictor, workload);
      random = new Random(selectorConfiguration.getRandomSeed());
      Collections.shuffle(this.steadyStateList, this.random);
   }

   @Override
   protected boolean _terminateExploration() {
      return false; //Delegate to previous predicate
   }

   @Override
   protected IntegerConfiguration _next() {
      IntegerConfiguration next = this.currentList.get(0);
      return next;
      // return this.currentList.get(0);
   }

   @Override
   public String toString() {
      return super.toString() + "__RandomIntegerConfigurationSelector{" +
            "random=" + random +
            '}';
   }
}
