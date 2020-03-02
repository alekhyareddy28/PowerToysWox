using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Wox.Plugin;

namespace Windows.Plugin.Indexer
{
    class Main : IPlugin
    {
        public List<Result> Query(Query query)
        {
            var result = new Result
            {
                Title = "Windows Indexer Plugin",
                SubTitle = $"Query: {query.Search}",
                IcoPath = "Images\\WindowsIndexerImg.bmp"
            };
            return new List<Result> { result };
        }

        public void Init(PluginInitContext context)
        {

        }
    }
}
