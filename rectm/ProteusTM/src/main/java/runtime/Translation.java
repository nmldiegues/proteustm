package runtime;

import java.io.IOException;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;

/**
 * Author: Diego Didona Email: didona@gsd.inesc-id.pt Websiste: www.cloudtm.eu Date: 24/05/12
 */
public class Translation {

   private HashMap<String, Integer> translation;
   private HashMap<Integer, String> headerIndices;
   private final String sep;

   public Translation(String header, String sep) throws IOException {
      this.sep = sep;
      this.translation = new HashMap<String, Integer>();
      this.headerIndices = new HashMap<Integer, String>();
      this.init(header, this.translation, headerIndices);
   }

   public int getParamIndex(String param) throws ParameterNotFoundException {
      Integer i = this.translation.get(param);
      if (i == null) {
         System.out.println(param + " not found");
         throw new ParameterNotFoundException(param);
      }
      return i;
   }

   private void init(String header, HashMap<String, Integer> translation, HashMap<Integer, String> indices) {
      String[] read = header.split(sep);
      int i = 0;
      for (String s : read) {
         if (translation.containsKey(s)) {
            System.out.println("Translation: Double entry in the header: " + s + " Going to put a random entry. (This works only for 1 collision) Header  is " + header);
            indices.put(i, "RANDOM");
            translation.put("RANDOM", i++); //I have to put this, so that the number of rows matches the sizes of translation
         } else {
            indices.put(i, s);
            translation.put(s, i++);
         }
      }
   }

   /**
    * Do not return the pointer!!!
    *
    * @return
    */
   public List<String> header() {
      LinkedList<String> ret = new LinkedList<>();
      for (int i = 0; i < translation.size(); i++) {
         ret.addLast(headerEntryFor(i));
      }
      return ret;
   }

   public int size() {
      return this.translation.size();
   }

   public boolean exist(String s) {
      return translation.containsKey(s);
   }

   public String headerEntryFor(int i) {
      return this.headerIndices.get(i);
   }

   public int indexForHeaderEntry(String entry) {
      for (int i : headerIndices.keySet()) {
         if (headerIndices.get(i).equals(entry))
            return i;
      }
      throw new RuntimeException(entry + " is not in the header!");
   }
}
