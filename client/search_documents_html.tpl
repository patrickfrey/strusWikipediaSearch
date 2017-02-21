{% extends "search_resultlist_html.tpl" %}

{% block relatedblock %}
{% if relatedterms %}
<div id="relatedresult">
<ul>
{% for result in relatedterms %}
<li onclick="parent.location='evalQuery.php?q={{ result.encvalue }}&s={{ scheme }}'">
<div id="related">
<div id="related_term">{{ result.value }}</div>
<div id="related_weight">{{ "%.4f" % result.weight }}</div>
</div>
</li>
{% end %}
</ul>
</div>
{% end %}
{% end %}

{% block nblinksblock %}
{% if nblinks %}
<div id="nblinkresult">
<ul>
{% for nblink in nblinks %}
{% set enclink = nblink.title.replace(' ','_') %}
<li onclick="parent.location='https://en.wikipedia.org/wiki/{{ enclink }}'">
<div id="nblink">
<div id="nblink_term">{{ nblink.title }}</div>
<div id="nblink_weight">{{ "%.4f" % nblink.weight }}</div>
</div>
</li>
{% end %}
</ul>
</div>
{% end %}
{% end %}

{% block resultblock %}
<div id="searchresult">
<ul>
{% for result in results %}
{% set enclink = result.title.replace(' ','_') %}
<li onclick="parent.location='https://en.wikipedia.org/wiki/{{ enclink }}'">
<h3>{{ result.title }}</h3>
<div id="rank">
{% if mode == "debug" %}
<div id="rank_docno">{{ result.docno }}</div>
<div id="rank_weight">{{ "%.4f" % result.weight }}</div>
{% end %}
{% if result.paratitle %}
<div id="rank_paratitle"> {% raw result.paratitle %} </div>
{% end %}
<div id="rank_abstract"> {% raw result.abstract %} </div>
</div>

{% if result.debuginfo %}
<div id="rank_debuginfo">
<input class="toggle-box" id="DebugInfo_usage" type="checkbox" >
<label for="DebugInfo_usage">Debug</label>
<pre>
{% raw result.debuginfo %}
</pre>
</div>
{% end %}
</li>
{% end %}
</ul>
</div>
{% end %}

