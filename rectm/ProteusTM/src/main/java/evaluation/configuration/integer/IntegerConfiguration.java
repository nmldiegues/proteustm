package evaluation.configuration.integer;

import evaluation.configuration.Configuration;

/**
 * @author Diego Didona
 * @email didona@gsd.inesc-id.pt
 * @since 12/09/14
 * <p/>
 * A configuration that is characterized by its offset in the training/test set
 * <p/>
 * NB: configuration indices have offset 0
 */
public class IntegerConfiguration implements Configuration {
   private int id;

   public IntegerConfiguration(int id) {
      this.id = id;
   }

   public int getId() {
      return id;
   }

   @Override
   public boolean equals(Object o) {
      if (this == o) return true;
      if (!(o instanceof IntegerConfiguration)) return false;

      IntegerConfiguration that = (IntegerConfiguration) o;

      if (id != that.id) return false;

      return true;
   }

   @Override
   public int hashCode() {
      return id;
   }

   @Override
   public String toString() {
      return "IntegerConfiguration{" +
            "id=" + id +
            '}';
   }
}
