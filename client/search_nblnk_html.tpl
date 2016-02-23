{% extends "search_resultlist_html.tpl" %}

{% block resultblock %}
<div id="searchresult">
<ul>
{% for result in results %}
{% set link = result.docid.replace(' ','_') %}
<li onclick="parent.location='https://en.wikipedia.org/wiki/{{ link }}'">
<h3>{{ result.title }}</h3>
<div id="rank">
<div id="rank_docno">{{ result.docno }}</div>
<div id="rank_weight">{{ "%.4f" % result.weight }}</div>
</div>
</li>
{% end %}
</ul>
</div>
{% end %}

