package evaluation.workload.integer;

import evaluation.common.RandomSelector;
import evaluation.workload.WorkloadSelector;

import java.util.List;
import java.util.Random;

/**
 * @author Diego Didona
 * @email didona@gsd.inesc-id.pt
 * @since 15/09/14
 */
public class IntegerRandomWorkloadSelector extends RandomSelector<IntegerWorkload> implements WorkloadSelector<IntegerWorkload> {
   public IntegerRandomWorkloadSelector(List<IntegerWorkload> workloadList, long seed) {
      super(workloadList, seed);
   }

   public IntegerRandomWorkloadSelector(List<IntegerWorkload> list, Random random) {
      super(list, random);
   }
}
