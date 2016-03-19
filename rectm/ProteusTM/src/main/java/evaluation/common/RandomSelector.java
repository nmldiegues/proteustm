package evaluation.common;

import java.util.Collections;
import java.util.List;
import java.util.Random;

/**
 * @author Diego Didona
 * @email didona@gsd.inesc-id.pt
 * @since 12/09/14
 */
public class RandomSelector<W> extends AbstractSelector<W> {

   private final Random random;

   protected RandomSelector(List<W> workloadList, long seed) {
      super(workloadList);
      //Shuffle only the steady-state list
      random = new Random(seed);
      Collections.shuffle(this.steadyStateList, this.random);
   }

   protected RandomSelector(List<W> bootstrapList, List<W> workloadList, long seed) {
      super(bootstrapList, workloadList);
      //Shuffle only the steady-state list
      random = new Random(seed);
      Collections.shuffle(this.steadyStateList, this.random);
   }

   public RandomSelector(List<W> list, Random random) {
      super(list);
      this.random = random;
      Collections.shuffle(this.steadyStateList, this.random);
   }

   @Override
   protected W _next() {
      return this.currentList.get(0);
   }

   @Override
   protected boolean terminateExploration() {
      return emptyList();
   }
}
